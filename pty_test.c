#include "pty_test.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#ifdef __APPLE__
#include <util.h>
#else
#include <pty.h>
#endif
#include <signal.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

pty_test_t *pty_test_create(void) {
    pty_test_t *pty = malloc(sizeof(pty_test_t));
    if (!pty) return NULL;
    
    memset(pty, 0, sizeof(pty_test_t));
    
    if (openpty(&pty->master_fd, &pty->slave_fd, NULL, NULL, NULL) == -1) {
        free(pty);
        return NULL;
    }
    
    /* Get slave terminal name */
    pty->slave_name = ttyname(pty->slave_fd);
    pty->is_open = true;
    
    /* Set non-blocking mode for master */
    int flags = fcntl(pty->master_fd, F_GETFL);
    fcntl(pty->master_fd, F_SETFL, flags | O_NONBLOCK);
    
    return pty;
}

void pty_test_destroy(pty_test_t *pty) {
    if (!pty) return;
    
    if (pty->is_open) {
        close(pty->master_fd);
        close(pty->slave_fd);
        pty->is_open = false;
    }
    
    free(pty);
}

int pty_test_spawn_process(pty_test_t *pty, const char *program, char *const argv[]) {
    if (!pty || !pty->is_open) return -1;
    
    pid_t pid = fork();
    if (pid == -1) {
        return -1;
    }
    
    if (pid == 0) {
        /* Child process */
        close(pty->master_fd);
        
        /* Make slave the controlling terminal */
        setsid();
        if (dup2(pty->slave_fd, STDIN_FILENO) == -1) exit(1);
        if (dup2(pty->slave_fd, STDOUT_FILENO) == -1) exit(1);
        if (dup2(pty->slave_fd, STDERR_FILENO) == -1) exit(1);
        
        close(pty->slave_fd);
        
        /* Execute program */
        execvp(program, argv);
        exit(1);
    }
    
    /* Parent process */
    close(pty->slave_fd);
    pty->slave_fd = -1;
    
    return (int)pid;
}

int pty_test_write(pty_test_t *pty, const char *input) {
    if (!pty || !pty->is_open || !input) return -1;
    
    size_t len = strlen(input);
    ssize_t written = write(pty->master_fd, input, len);
    
    return (written == (ssize_t)len) ? 0 : -1;
}

int pty_test_read(pty_test_t *pty, int timeout_ms) {
    if (!pty || !pty->is_open) return -1;
    
    struct pollfd pfd = {
        .fd = pty->master_fd,
        .events = POLLIN,
        .revents = 0
    };
    
    int poll_result = poll(&pfd, 1, timeout_ms);
    if (poll_result <= 0) return poll_result;
    
    if (pfd.revents & POLLIN) {
        ssize_t bytes_read = read(pty->master_fd, 
                                 pty->output_buffer + pty->output_len,
                                 PTY_MAX_OUTPUT - pty->output_len - 1);
        
        if (bytes_read > 0) {
            pty->output_len += (size_t)bytes_read;
            pty->output_buffer[pty->output_len] = '\0';
            return (int)bytes_read;
        }
    }
    
    return -1;
}

void pty_test_clear_output(pty_test_t *pty) {
    if (!pty) return;
    
    pty->output_len = 0;
    pty->output_buffer[0] = '\0';
}

int pty_test_set_winsize(pty_test_t *pty, unsigned short rows, unsigned short cols) {
    if (!pty || !pty->is_open) return -1;
    
    struct winsize ws = {
        .ws_row = rows,
        .ws_col = cols,
        .ws_xpixel = 0,
        .ws_ypixel = 0
    };
    
    return ioctl(pty->master_fd, TIOCSWINSZ, &ws);
}

int pty_test_send_key(pty_test_t *pty, char key) {
    if (!pty || !pty->is_open) return -1;
    
    return (write(pty->master_fd, &key, 1) == 1) ? 0 : -1;
}

int pty_test_send_ctrl_key(pty_test_t *pty, char key) {
    if (!pty || !pty->is_open) return -1;
    
    char ctrl_key = key & 0x1f;  /* Convert to control character */
    return (write(pty->master_fd, &ctrl_key, 1) == 1) ? 0 : -1;
}

bool pty_test_contains(pty_test_t *pty, const char *text) {
    if (!pty || !text) return false;
    
    return strstr(pty->output_buffer, text) != NULL;
}

bool pty_test_contains_escape_seq(pty_test_t *pty, const char *seq) {
    if (!pty || !seq) return false;
    
    /* Look for escape sequence in output */
    char *pos = pty->output_buffer;
    while ((pos = strchr(pos, '\033')) != NULL) {
        if (strncmp(pos, seq, strlen(seq)) == 0) {
            return true;
        }
        pos++;
    }
    
    return false;
}

int pty_test_count_lines(pty_test_t *pty) {
    if (!pty) return -1;
    
    int lines = 1;
    for (size_t i = 0; i < pty->output_len; i++) {
        if (pty->output_buffer[i] == '\n') {
            lines++;
        }
    }
    
    return lines;
}

bool pty_test_cursor_at(pty_test_t *pty, int row, int col) {
    if (!pty) return false;
    
    /* Look for cursor position escape sequence */
    char expected[32];
    snprintf(expected, sizeof(expected), "\x1b[%d;%dH", row, col);
    
    return pty_test_contains_escape_seq(pty, expected);
}

void pty_assert(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "PTY Test Failed: %s\n", message ? message : "assertion failed");
        exit(1);
    }
}

void pty_assert_contains(pty_test_t *pty, const char *text, const char *message) {
    if (!pty_test_contains(pty, text)) {
        fprintf(stderr, "PTY Test Failed: %s\n", message ? message : "text not found");
        fprintf(stderr, "Expected to find: '%s'\n", text);
        fprintf(stderr, "Actual output: '%s'\n", pty->output_buffer);
        exit(1);
    }
}

void pty_assert_not_contains(pty_test_t *pty, const char *text, const char *message) {
    if (pty_test_contains(pty, text)) {
        fprintf(stderr, "PTY Test Failed: %s\n", message ? message : "unexpected text found");
        fprintf(stderr, "Should not contain: '%s'\n", text);
        fprintf(stderr, "Actual output: '%s'\n", pty->output_buffer);
        exit(1);
    }
}
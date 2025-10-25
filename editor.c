#define _POSIX_C_SOURCE 200112L
#include "editor.h"
#include "ds.h"
#include "render.h"
#include "term.h"
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define INITIAL_CAPACITY 128
#define PT_MAX_HEADER_SIZE 4

PTState *pt_new_glob_state(pt_str *filename) {
        PTState *state = calloc(1, sizeof(PTState));

        pt_str *content = pt_str_new();

        state->content = content;
        state->filename = filename;
        state->is_censored = false;
        pt_refresh_terminal_state(state);

        return state;
}

void pt_refresh_terminal_state(PTState *state) {
        // Get the rows and columns
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        state->rows = w.ws_row;
        state->cols = w.ws_col;
}

static void pt_add_char(PTState *state, char c) {
        pt_str_append_char(state->content, c);
}

static void pt_delete_char(PTState *state) {
        pt_str_delete_char(state->content);
}

static char pt_read_key(void) {
        long nread;
        char c;
        while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
                if (nread == -1 && errno != EAGAIN)
                        pt_die("read");
        }
        return c;
}

void pt_save_to_file(PTState *state, const pt_str *filename) {
        FILE *file = fopen(filename->data, "w");
        if (file) {
                fwrite(state->content->data, 1, state->content->len, file);
                fclose(file);
        } else {
                perror("Failed to open file for writing");
        }
}

void pt_load_from_file(PTState *state, const pt_str *filename) {
        FILE *file = fopen(filename->data, "r");
        if (file) {
                fseek(file, 0, SEEK_END);
                size_t size = (size_t)ftell(file);
                fseek(file, 0, SEEK_SET);

                char *file_data = malloc(size + 1);
                if (fread(file_data, 1, size, file) != size) {
                        free(file_data);
                        fclose(file);
                        return;
                }
                file_data[size] = 0;
                fclose(file);

                // TODO: this is not great
                pt_str_free(state->content);
                pt_str_init(state->content);
                pt_str_append(state->content, file_data);

                free(file_data);
        } else {
                perror("Failed to open file for reading");
        }
}

void pt_handle_key_press(PTState *state) {
        char c = pt_read_key();
        switch (c) {
        case '\r': // Enter key
                pt_add_char(state, '\n');
                break;
        case '\x7f': // Backspace key
                pt_delete_char(state);
                break;
        case CTRL_KEY('q'): // Ctrl-Q
                exit(0);
                break;
        case CTRL_KEY('s'): // Ctrl-S
                pt_save_to_file(state, state->filename);

                pt_move_cursor(1, 1);
                printf("Saved to %s", state->filename->data);
                pt_move_cursor(2, 1);
                fflush(stdout);
                sleep(1);
                break;
        case CTRL_KEY('c'):
                state->is_censored = !state->is_censored;
                break;
        default:
                pt_add_char(state, c);
                break;
        }
}

#define ARRAY_SIZE(arr)                                                        \
        ((sizeof(arr) / sizeof((arr)[0])) /                                    \
         ((size_t)(!(sizeof(arr) % sizeof((arr)[0])))))

void pt_splash_screen(PTState *state) {

        // clang-format off
  	const char *lines[] = {
      	"                                \r\n",
      	"                                \r\n",
      	"    ╔══════════════════════╗    \r\n",
      	"    ║                      ║    \r\n",
      	"    ║   Welcome to PORTA   ║    \r\n",
      	"    ║                      ║    \r\n",
      	"    ║──────────────────────║    \r\n",
      	"    ║                      ║    \r\n",
      	"    ║  ctrl + q to quit    ║    \r\n",
      	"    ║                      ║    \r\n",
      	"    ║  ctrl + s to save    ║    \r\n",
      	"    ║                      ║    \r\n",
      	"    ║  ctrl + c to censor  ║    \r\n",
      	"    ║                      ║    \r\n",
      	"    ╚══════════════════════╝    \r\n",
      	"                                \r\n",
      	"                                \r\n"};
        // clang-format on

        size_t lines_size = ARRAY_SIZE(lines);

        unsigned short middle_row =
                state->rows / 2 - (unsigned short)lines_size / 2;

        size_t len = strlen(lines[0]);
        unsigned short col = (unsigned short)((state->cols - len) / 2);
        for (unsigned short i = 0; i < lines_size; i++) {
                pt_move_cursor(middle_row + i, col);
                printf("%s", lines[i]);
        }
        pt_move_cursor(2, 1);
        fflush(stdout);
        // Wait for a key press
        pt_handle_key_press(state);
}

#ifdef PT_TEST
#endif /* PT_TEST */

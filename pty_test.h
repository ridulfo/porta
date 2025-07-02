#ifndef PTY_TEST_H
#define PTY_TEST_H

#include <stddef.h>
#include <stdbool.h>

#define PTY_MAX_OUTPUT 8192
#define PTY_MAX_INPUT 1024

typedef struct {
    int master_fd;
    int slave_fd;
    char *slave_name;
    char output_buffer[PTY_MAX_OUTPUT];
    size_t output_len;
    bool is_open;
} pty_test_t;

/* PTY management */
pty_test_t *pty_test_create(void);
void pty_test_destroy(pty_test_t *pty);
int pty_test_spawn_process(pty_test_t *pty, const char *program, char *const argv[]);

/* Input/Output operations */
int pty_test_write(pty_test_t *pty, const char *input);
int pty_test_read(pty_test_t *pty, int timeout_ms);
void pty_test_clear_output(pty_test_t *pty);

/* Terminal simulation */
int pty_test_set_winsize(pty_test_t *pty, unsigned short rows, unsigned short cols);
int pty_test_send_key(pty_test_t *pty, char key);
int pty_test_send_ctrl_key(pty_test_t *pty, char key);

/* Output analysis */
bool pty_test_contains(pty_test_t *pty, const char *text);
bool pty_test_contains_escape_seq(pty_test_t *pty, const char *seq);
int pty_test_count_lines(pty_test_t *pty);
bool pty_test_cursor_at(pty_test_t *pty, int row, int col);

/* Test assertions */
void pty_assert(bool condition, const char *message);
void pty_assert_contains(pty_test_t *pty, const char *text, const char *message);
void pty_assert_not_contains(pty_test_t *pty, const char *text, const char *message);

#endif
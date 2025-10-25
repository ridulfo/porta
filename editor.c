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
#define TEXT_WIDTH 80

PTState *pt_new_glob_state(pt_str *filename) {
        PTState *state = calloc(1, sizeof(PTState));
        // Get the rows and columns
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

        pt_str *content = pt_str_new();

        state->rows = w.ws_row;
        state->cols = w.ws_col;
        state->content = content;
        state->filename = filename;
        state->is_censored = false;

        return state;
}

void pt_add_char(PTState *state, char c) {
        pt_str_append_char(state->content, c);
}

void pt_delete_char(PTState *state) { pt_str_delete_char(state->content); }

static const char *pt_skip_escape_sequence(const char *input) {
        // TODO: add char** first and last to parameters so that we can count
        // what is inside the escaped
        if (*input != '\033')
                return input;
        input++; // Skip ESC

        if (*input == '[') {
                // ANSI CSI sequence
                while (*input && strchr("mHJ", *input) == NULL)
                        input++;
        } else if (*input == ']') {
                // OSC sequence (like Kitty)
                while (*input && *input != '\a' &&
                       !(*input == '\033' && input[1] == '\\'))
                        input++;
                if (*input == '\a')
                        input++;
                else if (*input == '\033')
                        input += 2;
                return input;
        }
        if (*input)
                input++; // Skip final byte
        return input;
}

/**
 * Splits `input` into wrapped lines of at most 80 characters.
 * Allocates and fills an array of `pt_str`, returned via `lines_out`.
 * Returns the number of lines, or 0 on failure.
 */
static int pt_split_lines(const pt_str *input, pt_str **lines_out) {
        if (!input || !lines_out)
                return -1;

        // First pass: count needed lines
        size_t char_count = 0, lines_count = 1;
        for (const char *c = input->data; *c; ++c) {
                if (*c == '\033') {
                        const char *after_escape = pt_skip_escape_sequence(c);
                        c = after_escape - 1;
                } else if (char_count >= TEXT_WIDTH || *c == '\n') {
                        lines_count++;
                        char_count = 0;
                } else {
                        char_count++;
                }
        }

        // Allocate the array
        pt_str *lines = calloc(lines_count, sizeof(pt_str));
        if (!lines)
                return -1;

        for (size_t i = 0; i < lines_count; i++) {
                pt_str_init(&lines[i]);
        }

        // Second pass: fill the lines
        size_t line_index = 0;
        size_t visible_char_count = 0; // Track visible characters separately
        for (const char *c = input->data; *c; ++c) {
                pt_str *line = &lines[line_index];

                if (*c == '\033') {
                        const char *after_escape = pt_skip_escape_sequence(c);
                        // Copy entire escape sequence
                        while (c < after_escape) {
                                pt_str_append_char(line, *c);
                                c++;
                        }
                        c--; // Adjust for loop increment
                } else if (visible_char_count >= TEXT_WIDTH || *c == '\n') {
                        line_index++;
                        if (line_index >= lines_count)
                                break;          // Prevent overflow
                        visible_char_count = 0; // Reset visible char count
                        if (*c == '\n')
                                continue;
                        line = &lines[line_index]; // Update line pointer
                        pt_str_append_char(line, *c);
                        visible_char_count++;
                } else {
                        pt_str_append_char(line, *c);
                        visible_char_count++;
                }
        }

        *lines_out = lines;
        return (int)lines_count;
}

/**
 * Render the current state: clear screen, wrap text,
 * then print it centered like a typewriter effect.
 */
void pt_render_state(PTState *state) {
        pt_clear_screen();
        pt_str *content = pt_str_from(state->content->data);

        if (state->is_censored)
                censor_text(content);

        const char *term = getenv("TERM");
        if (term && strcmp(term, "xterm-kitty") == 0) {
                pt_str *formatted = pt_format_string(content);
                pt_str_free(content);
                content = formatted;
        }

        pt_str *lines = {0};
        int line_count = pt_split_lines(content, &lines);
        if (line_count < 0)
                pt_die("split lines");

        const unsigned short center_row = (unsigned short)(state->rows - 1) / 2;
        const unsigned short start_col =
                (unsigned short)(state->cols - TEXT_WIDTH) / 2;
        const unsigned short max_rows = center_row;

        // Start at the back and print one row at a time
        // move up after every print
        unsigned short i = 0;
        while (i < line_count && i < max_rows) {
                unsigned short row_idx = center_row - i;

                unsigned short line_idx = (unsigned short)(line_count - 1 - i);
                pt_move_cursor(row_idx, start_col);
                printf("%s", lines[line_idx].data);
                i++;
        }
        // Print the center line again to get the cursor right
        pt_move_cursor(center_row, start_col);
        printf("%s", lines[line_count - 1].data);

        // Cleanup
        for (int j = 0; j < line_count; j++) {
                pt_str_free(&lines[j]);
        }
        free(lines);
        pt_str_free(content);
        free(content);

        fflush(stdout);
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

void pt_process_key_press(PTState *state) {
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

void pt_move_cursor(unsigned short row, unsigned short col) {
        if (row >= 1 && col >= 1) {
                printf("\x1b[%d;%dH", row, col);
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
        pt_process_key_press(state);
}

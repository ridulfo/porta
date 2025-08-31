#define _POSIX_C_SOURCE 200112L
#include "editor.h"
#include "ds.h"
#include "term.h"
#include <ctype.h>
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

PTState pt_global_state = {0};

void pt_init_glob_state(pt_str *filename) {
        // Get the rows and columns
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

        pt_str *content = pt_str_new();

        pt_global_state = (PTState){
                .rows = w.ws_row,
                .cols = w.ws_col,
                .content = content,
                .filename = filename,
                .is_censored = false,
        };
}

void pt_add_char(char c) { pt_str_append_char(pt_global_state.content, c); }

void pt_delete_char(void) { pt_str_delete_char(pt_global_state.content); }

/** Replaces all the alpha-num characters up to the last one with an asterisk */
static void censor_text(pt_str *text) {
        size_t len = text->len;
        if (len == 0)
                return;

        for (size_t i = 0; i < len - 1; i++) {
                char *c = &text->data[i];
                if (isalnum(*c)) {
                        *c = '*';
                }
        }
}

static pt_str *pt_format_string(const pt_str *input) {
        pt_str *out = pt_str_new();

        const char *p = input->data;
        while (*p) {
                if (*p == '#') {
                        size_t raw_count = strspn(p, "#");
                        char *nl = strchr(p, '\n');

                        // Heading must have a space after # and have a new line
                        if (p[raw_count] != ' ' || (!nl)) {
                                pt_str_append_char(out, *p);
                                p++;
                                continue;
                        }

                        // min(raw_count, PT_MAX_HEADER_SIZE)
                        size_t count = raw_count > PT_MAX_HEADER_SIZE
                                               ? PT_MAX_HEADER_SIZE
                                               : raw_count;

                        // skip the hashes and the space
                        p += raw_count + 1;
                        const char *text_start = p;
                        size_t text_len = (size_t)(nl - text_start);

                        size_t level = PT_MAX_HEADER_SIZE - count;

                        /* emit control sequence */
                        char ctrl[128];
                        int n = snprintf(ctrl, sizeof(ctrl),
                                         "\033]66;s=%zu;%.*s\a", level,
                                         (int)text_len, text_start);
                        if (n > 0 && n < (int)sizeof(ctrl)) {
                                pt_str_append(out, ctrl);
                        }

                        for (size_t k = 0; k <= level - 1; k++) {
                                pt_str_append_char(out, '\n');
                        }

                        p = nl + 1;
                        continue;
                }

                /* regular character */
                pt_str_append_char(out, *p);
                p++;
        }

        return out;
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
                if (char_count >= TEXT_WIDTH || *c == '\n') {
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
        for (const char *c = input->data; *c; ++c) {
                pt_str *line = &lines[line_index];
                if (line->len >= TEXT_WIDTH || *c == '\n') {
                        line_index++;
                        if (*c == '\n')
                                continue;
                }
                pt_str_append_char(line, *c);
        }

        *lines_out = lines;
        return (int)lines_count;
}

/**
 * Render the current state: clear screen, wrap text,
 * then print it centered like a typewriter effect.
 */
void pt_render_state(void) {
        pt_clear_screen();
        pt_str *content = pt_str_from(pt_global_state.content->data);

        if (pt_global_state.is_censored)
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

        const unsigned short center_row =
                (unsigned short)(pt_global_state.rows - 1) / 2;
        const unsigned short start_col =
                (unsigned short)(pt_global_state.cols - TEXT_WIDTH) / 2;
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

void pt_save_to_file(const pt_str *filename) {
        FILE *file = fopen(filename->data, "w");
        if (file) {
                fwrite(pt_global_state.content->data, 1,
                       pt_global_state.content->len, file);
                fclose(file);
        } else {
                perror("Failed to open file for writing");
        }
}

void pt_load_from_file(const pt_str *filename) {
        FILE *file = fopen(filename->data, "r");
        if (file) {
                fseek(file, 0, SEEK_END);
                size_t size = (size_t)ftell(file);
                fseek(file, 0, SEEK_SET);

                char *file_data = malloc(size + 1);
                fread(file_data, 1, size, file);
                file_data[size] = 0;
                fclose(file);

                // TODO: this is not great
                pt_str_free(pt_global_state.content);
                pt_str_init(pt_global_state.content);
                pt_str_append(pt_global_state.content, file_data);

                free(file_data);
        } else {
                perror("Failed to open file for reading");
        }
}

void pt_process_key_press(void) {
        char c = pt_read_key();
        switch (c) {
        case '\r': // Enter key
                pt_add_char('\n');
                break;
        case '\x7f': // Backspace key
                pt_delete_char();
                break;
        case CTRL_KEY('q'): // Ctrl-Q
                exit(0);
                break;
        case CTRL_KEY('s'): // Ctrl-S
                pt_save_to_file(pt_global_state.filename);

                pt_move_cursor(1, 1);
                printf("Saved to %s", pt_global_state.filename->data);
                pt_move_cursor(2, 1);
                fflush(stdout);
                sleep(1);
                break;
        case CTRL_KEY('c'):
                pt_global_state.is_censored = !pt_global_state.is_censored;
                break;
        default:
                pt_add_char(c);
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

void pt_splash_screen(void) {

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
                pt_global_state.rows / 2 - (unsigned short)lines_size / 2;

        size_t len = strlen(lines[0]);
        unsigned short col = (unsigned short)((pt_global_state.cols - len) / 2);
        for (unsigned short i = 0; i < lines_size; i++) {
                pt_move_cursor(middle_row + i, col);
                printf("%s", lines[i]);
        }
        pt_move_cursor(2, 1);
        fflush(stdout);
        // Wait for a key press
        pt_process_key_press();
}

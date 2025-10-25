#include <ctype.h>
#define _POSIX_C_SOURCE 200112L
#include "ds.h"
#include "editor.h"
#include "render.h"
#include "term.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PT_MAX_HEADER_SIZE 4
#define TEXT_WIDTH 80

void censor_text(pt_str *text) {
        size_t len = text->len;
        if (len == 0)
                return;

        for (size_t i = 0; i < len - 1; i++) {
                char *c = &text->data[i];
                if (isalnum(*c)) {
                        *c = 'X';
                }
        }
}

/**
 * Takes an input and output string and returns a pointer to after the heading
 *
 * `input` needs to point to the first pound (it is fine if turns out to be a
 * tag)
 */
static const char *pt_format_heading(const char *input, pt_str *out) {
        size_t raw_count = strspn(input, "#");
        char *nl = strchr(input, '\n');

        // Heading must have a space after # and have a new line
        if (input[raw_count] != ' ' || (!nl)) {
                pt_str_append_char(out, *input);
                return input + 1;
        }

        // min(raw_count, PT_MAX_HEADER_SIZE)
        size_t count =
                raw_count > PT_MAX_HEADER_SIZE ? PT_MAX_HEADER_SIZE : raw_count;

        // skip the hashes and the space
        input += raw_count + 1;
        const char *text_start = input;
        size_t text_len = (size_t)(nl - text_start);

        size_t level = PT_MAX_HEADER_SIZE - count;

        /* emit control sequence */
        char ctrl[128];
        int n = snprintf(ctrl, sizeof(ctrl), "\033]66;s=%zu;%.*s\a", level,
                         (int)text_len, text_start);

        // Add the characters from the control sequence
        if (n > 0 && n < (int)sizeof(ctrl)) {
                pt_str_append(out, ctrl);
        }

        // Add the appropriate number of new lines
        for (size_t k = 0; k <= level - 1; k++) {
                pt_str_append_char(out, '\n');
        }

        input = nl + 1;
        return input;
}

static const char *pt_format_bold(const char *input, pt_str *out) {
        if (input[0] != '*' || input[1] != '*') {
                return input;
        }
        const char *text_start = input + 2;
        const char *end_marker = strstr(text_start, "**");

        if (end_marker == NULL) {
                // Did not find enclosing **
                pt_str_append_char(out, '*');
                pt_str_append_char(out, '*');
                return input + 2;
        }
        size_t text_len = (size_t)(end_marker - text_start);

        char ctrl[128];
        int n = snprintf(ctrl, sizeof(ctrl), "\033[1m%.*s\033[0m",
                         (int)text_len, text_start);

        if (n > 0 && n < (int)sizeof(ctrl)) {
                pt_str_append(out, ctrl);
        }
        input = end_marker + 2;

        return input;
}
static const char *pt_format_wikilink(const char *input, pt_str *out) {
        const char *text_start = input + 2;
        const char *end_marker = strstr(text_start, "]]");
        if (end_marker == NULL) {
                // Did not find enclosing ]]
                pt_str_append(out, "[[");
                return input + 2;
        }

        // Look for the last | character within the wikilink
        const char *pipe_pos = NULL;
        const char *p = text_start;
        while (p < end_marker) {
                if (*p == '|') {
                        pipe_pos = p;
                }
                p++;
        }

        // If we found a |, render text after it; otherwise render all text
        const char *render_start = pipe_pos ? pipe_pos + 1 : text_start;
        size_t text_len = (size_t)(end_marker - render_start);

        char ctrl[128];
        int n = snprintf(ctrl, sizeof(ctrl), "\033[4m%.*s\033[0m",
                         (int)text_len, render_start);

        if (n > 0 && n < (int)sizeof(ctrl)) {
                pt_str_append(out, ctrl);
        }

        input = end_marker + 2;
        return input;
}

pt_str *pt_format_string(const pt_str *input) {
        pt_str *out = pt_str_new();

        const char *p = input->data;
        while (*p) {
                // Either first character in the input is a pound or pound has
                // been preceeded by a newline
                if (p[0] == '#' && (p == input->data || *(p - 1) == '\n')) {
                        p = pt_format_heading(p, out);
                        continue;
                } else if (p[0] == '*' && p[1] == '*') {
                        p = pt_format_bold(p, out);
                        continue;
                } else if (p[0] == '[' && p[1] == '[') {
                        p = pt_format_wikilink(p, out);
                        continue;
                }

                /* regular character */
                pt_str_append_char(out, *p);
                p++;
        }

        return out;
}

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
                free(content);
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


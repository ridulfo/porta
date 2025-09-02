#include <ctype.h>
#define _POSIX_C_SOURCE 200112L
#include "ds.h"
#include "render.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define PT_MAX_HEADER_SIZE 4

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

pt_str *pt_format_string(const pt_str *input) {
        pt_str *out = pt_str_new();

        const char *p = input->data;
        while (*p) {
                // Either first character in the input is a pound or pound has
                // been preceeded by a newline
                if (p[0] == '#' && (p == input->data || *(p - 1) == '\n')) {
                        p = pt_format_heading(p, out);
                        continue;
                } else if (*p == '*' && *(p + 1) == '*') {
                        p = pt_format_bold(p, out);
                        continue;
                }

                /* regular character */
                pt_str_append_char(out, *p);
                p++;
        }

        return out;
}

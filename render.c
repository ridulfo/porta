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
                        *c = '*';
                }
        }
}

/**
 * Takes an input and output string and returns a pointer to how
 */
static const char *pt_format_heading(const char *input, pt_str *out) {

        input++;
        size_t raw_count = strspn(input, "#");
        char *nl = strchr(input, '\n');

        // Heading must have a space after # and have a new line
        if (input[raw_count] != ' ' || (!nl)) {
                pt_str_append_char(out, '\n');
                pt_str_append_char(out, *input);
                input++;
                return input;
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

        // Add the first new line that was used in the checking
        pt_str_append_char(out, '\n');

        // Add the characters from the control sequence
        if (n > 0 && n < (int)sizeof(ctrl)) {
                pt_str_append(out, ctrl);
        }

        // Add the appropriate number of new lines
        for (size_t k = 0; k <= level - 1; k++) {
                pt_str_append_char(out, '\n');
        }

        input = nl + 2;
        return input;
}

pt_str *pt_format_string(const pt_str *input) {
        pt_str *out = pt_str_new();

        const char *p = input->data;
        while (*p) {
                if (*p == '\n' && *(p + 1) == '#') {
                        p = pt_format_heading(p, out);
                        continue;
		}
                /* regular character */
                pt_str_append_char(out, *p);
                p++;
        }

        return out;
}

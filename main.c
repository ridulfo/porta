#define _POSIX_C_SOURCE 200112L
#include "ds.h"
#include "editor.h"
#include "term.h"
#include <stdio.h>
#include <unistd.h>

// Good resources
// https://gist.github.com/delameter/b9772a0bf19032f977b985091f0eb5c1
// https://vt100.net/annarbor/aaa-ug/section6.html

int main(int argc, char *argv[]) {
        if (argc < 2) {
                fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
                return 1;
        }
        pt_str *filename = pt_str_from(argv[1]);
        pt_init_term();
        pt_init_glob_state(filename);
        pt_load_from_file(filename);

        pt_render_state();
        pt_splash_screen();

        while (1) {
                pt_render_state();
                pt_process_key_press();
        }

        return 0;
}

#include "editor.h"
#include "errno.h"
#include "term.h"
#include <signal.h>
#include <unistd.h>

#define _POSIX_C_SOURCE 200112L

// Good resources
// https://gist.github.com/delameter/b9772a0bf19032f977b985091f0eb5c1
// https://vt100.net/annarbor/aaa-ug/section6.html

static void autosave_on_inactive(int _) {
  pt_save_to_file(pt_global_state.filename);
  pt_move_cursor(1, 1);
  printf("Autosaved to %s", pt_global_state.filename);
  pt_move_cursor(2, 1);
  fflush(stdout);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    return 1;
  }
  char *filename = argv[1];
  pt_init_term();
  pt_init_glob_state(filename);
  pt_load_from_file(filename);

  pt_render_state();
  pt_splash_screen();

  signal(SIGALRM, autosave_on_inactive);

  while (1) {
    pt_render_state();
    pt_process_key_press();
    alarm(5);
  }

  return 0;
}

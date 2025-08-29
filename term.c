#include "term.h"
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>
#define _POSIX_C_SOURCE 200112L

static struct termios orig_termios;

void pt_die(const char *s) {
  perror(s);
  exit(1);
}

static void pt_switch_to_alt_buffer(void) {
  printf("\033[?1049h\033[H");
  fflush(stdout);
}

static void pt_switch_from_alt_buffer(void) {
  printf("\033[?1049l");
  fflush(stdout);
}

static void pt_disable_raw_mode(void) {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
    pt_die("tcsetattr");
  pt_switch_from_alt_buffer();
}

static void pt_enable_raw_mode(void) {
  pt_switch_to_alt_buffer();

  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
    pt_die("tcgetattr");

  atexit(pt_disable_raw_mode); // Auto cleanup on exit

  struct termios raw = orig_termios;
  raw.c_lflag &= (tcflag_t) ~(ECHO | ICANON | ISIG | IEXTEN);
  raw.c_iflag &= (tcflag_t) ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
  raw.c_cflag |= (CS8);
  raw.c_oflag &= (tcflag_t) ~(OPOST);

  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    pt_die("tcsetattr");
}

void pt_init_term(void) {

  if (!setlocale(LC_CTYPE, "")) { // Empty string for all locales
    pt_die("setlocale");
  }
  fwide(stdout, 1); // Set the wide character orientation for stdout

  pt_enable_raw_mode();
}

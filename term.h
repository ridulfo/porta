#ifndef TERM_H
#define TERM_H
#include <stddef.h>

#define pt_clear_screen() printf("\033[H\033[J")

void pt_die(const char *s);
void pt_init_term(void);

void pt_move_cursor(unsigned short row, unsigned short col);

#endif

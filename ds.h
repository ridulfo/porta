#ifndef DS_H
#define DS_H

#include <stddef.h>
typedef struct {
  char *data;
  size_t len;
  size_t cap;
} pt_str;

pt_str *pt_str_new(void);
pt_str *pt_str_from(const char *str);
int pt_str_init(pt_str *s);
void pt_str_free(pt_str *s);
int pt_str_append(pt_str *s, const char *suffix);
void pt_str_append_char(pt_str *s, char c);
void pt_str_delete_char(pt_str *s);

#endif

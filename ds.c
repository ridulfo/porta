#include "ds.h"
#include "term.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

pt_str *pt_str_new(void) {
  pt_str *str = malloc(sizeof(pt_str));
  pt_str_init(str);
  return str;
}

pt_str *pt_str_from(char *str) {
  pt_str *new_str = pt_str_new();
  pt_str_append(new_str, str);
  return new_str;
}

void pt_str_init(pt_str *s) {
  s->cap = 16;
  s->len = 0;
  s->data = malloc(s->cap);
  if (s->data)
    s->data[0] = '\0';
}

void pt_str_free(pt_str *s) {
  free(s->data);
  s->data = NULL;
  s->len = 0;
  s->cap = 0;
}

void pt_str_append(pt_str *s, const char *suffix) {
  if (!s || !s->data || s->cap == 0) {
    fprintf(stderr, "pt_str_append: uninitialized pt_str\n");
    exit(EXIT_FAILURE);
  }

  size_t suffix_len = strlen(suffix);
  size_t required = s->len + suffix_len + 1;

  if (required > s->cap) {
    size_t new_cap = s->cap;
    while (new_cap < required) {
      new_cap *= 2;
    }
    char *new_data = realloc(s->data, new_cap);
    if (!new_data) {
      pt_die("realloc");
    }
    s->data = new_data;
    s->cap = new_cap;
  }

  memcpy(s->data + s->len, suffix, suffix_len);
  s->len += suffix_len;
  s->data[s->len] = '\0';
}

void pt_str_append_char(pt_str *s, char c) {
  char buf[2] = {c, '\0'};
  pt_str_append(s, buf);
}

void pt_str_delete_char(pt_str *s) {
  if (s->len > 0) {
    s->data[--s->len] = '\0';
  }
}

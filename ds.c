#define _POSIX_C_SOURCE 200112L
#include "ds.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

pt_str *pt_str_new(void) {
        pt_str *str = malloc(sizeof(pt_str));
        if (!str)
                return NULL;

        if (pt_str_init(str) != 0) {
                free(str);
                return NULL;
        }

        return str;
}

pt_str *pt_str_from(const char *str) {
        pt_str *new_str = pt_str_new();
        pt_str_append(new_str, str);
        return new_str;
}

int pt_str_init(pt_str *s) {
        s->cap = 16;
        s->len = 0;
        s->data = malloc(s->cap);
        if (!s->data)
                return -1;
        s->data[0] = '\0';
        return 0;
}

void pt_str_free(pt_str *s) {
        free(s->data);
        s->data = NULL;
        s->len = 0;
        s->cap = 0;
}

int pt_str_append(pt_str *s, const char *suffix) {
        if (!s || !s->data || s->cap == 0) {
                return -1;
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
                        return -1;
                }
                s->data = new_data;
                s->cap = new_cap;
        }

        memcpy(s->data + s->len, suffix, suffix_len);
        s->len += suffix_len;
        s->data[s->len] = '\0';

        return (int)suffix_len;
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

#ifdef PT_TEST

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Helpers to avoid malloc‐leaks */
static pt_str *mk(void) { return pt_str_new(); }
static void fk(pt_str *s) {
        pt_str_free(s);
        free(s);
}

/* Existing tests */
static void test_new(void) {
        pt_str *s = mk();
        assert(s != NULL);
        assert(s->data != NULL);
        assert(s->len == 0);
        assert(s->cap >= 16);
        assert(strcmp(s->data, "") == 0);
        fk(s);
        putchar('.');
}

static void test_from(void) {
        pt_str *s = pt_str_from("hello");
        assert(s->len == 5);
        assert(strcmp(s->data, "hello") == 0);
        fk(s);
        putchar('.');
}

static void test_append(void) {
        pt_str *s = mk();
        int rc = pt_str_append(s, "abc");
        assert(rc == 3);
        assert(s->len == 3);
        assert(strcmp(s->data, "abc") == 0);
        rc = pt_str_append(s, "");
        assert(rc == 0);
        assert(s->len == 3);
        assert(strcmp(s->data, "abc") == 0);
        fk(s);
        putchar('.');
}

static void test_resize(void) {
        pt_str *s = mk();
        char buf[1024];
        memset(buf, 'x', sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        int rc = pt_str_append(s, buf);
        assert(rc == 1023);
        assert(s->len == 1023);
        assert(s->cap >= 1024);
        for (size_t i = 0; i < s->len; i++)
                assert(s->data[i] == 'x');
        fk(s);
        putchar('.');
}

static void test_append_char(void) {
        pt_str *s = mk();
        pt_str_append_char(s, 'A');
        pt_str_append_char(s, 'B');
        pt_str_append_char(s, 'C');
        assert(s->len == 3);
        assert(strcmp(s->data, "ABC") == 0);
        fk(s);
        putchar('.');
}

static void test_delete_char(void) {
        pt_str *s = pt_str_from("XYZ");
        pt_str_delete_char(s);
        assert(s->len == 2);
        assert(strcmp(s->data, "XY") == 0);
        pt_str_delete_char(s);
        pt_str_delete_char(s);
        pt_str_delete_char(s); /* no-op */
        assert(s->len == 0);
        assert(strcmp(s->data, "") == 0);
        fk(s);
        putchar('.');
}

static void test_free(void) {
        pt_str *s = mk();
        pt_str_append(s, "foo");
        pt_str_free(s);
        assert(s->data == NULL);
        assert(s->len == 0);
        assert(s->cap == 0);
        free(s);
        putchar('.');
}

/* New edge‐case tests */

static void test_delete_on_empty(void) {
        pt_str *s = mk();
        pt_str_delete_char(s);
        assert(s->len == 0);
        assert(strcmp(s->data, "") == 0);
        fk(s);
        putchar('.');
}

static void test_from_empty(void) {
        pt_str *s = pt_str_from("");
        assert(s->len == 0);
        assert(strcmp(s->data, "") == 0);
        assert(s->cap >= 16);
        fk(s);
        putchar('.');
}

static void test_multiple_append_char(void) {
        pt_str *s = mk();
        for (int i = 0; i < 100; i++) {
                pt_str_append_char(s, 'x');
        }
        assert(s->len == 100);
        for (int i = 0; i < 100; i++)
                assert(s->data[i] == 'x');
        assert(s->data[100] == '\0');
        fk(s);
        putchar('.');
}

static void test_capacity_boundaries(void) {
        pt_str *s = mk();
        char a15[16];
        memset(a15, 'a', 15);
        a15[15] = '\0';
        pt_str_append(s, a15);
        assert(s->len == 15);
        assert(s->cap == 16);

        pt_str_append_char(s, 'b');
        assert(s->len == 16);
        assert(s->cap == 32);

        char c15[16];
        memset(c15, 'c', 15);
        c15[15] = '\0';
        pt_str_append(s, c15);
        assert(s->len == 31);
        assert(s->cap == 32);

        pt_str_append_char(s, 'd');
        assert(s->len == 32);
        assert(s->cap == 64);

        fk(s);
        putchar('.');
}

static void test_double_free(void) {
        pt_str *s = mk();
        pt_str_append(s, "abc");
        pt_str_free(s);
        pt_str_free(s); /* second free is a no-op */
        assert(s->data == NULL);
        assert(s->len == 0);
        assert(s->cap == 0);
        free(s);
        putchar('.');
}

static void test_append_after_delete(void) {
        pt_str *s = pt_str_from("hello");
        pt_str_delete_char(s);
        pt_str_delete_char(s);
        pt_str_append(s, "p!");
        assert(s->len == 5);
        assert(strcmp(s->data, "help!") == 0);
        fk(s);
        putchar('.');
}

static void test_chain_append(void) {
        pt_str *s = mk();
        pt_str_append(s, "foo");
        pt_str_append_char(s, '-');
        pt_str_append(s, "bar");
        assert(s->len == 7);
        assert(strcmp(s->data, "foo-bar") == 0);
        fk(s);
        putchar('.');
}

static void test_init_on_stack(void) {
        pt_str s;
        pt_str_init(&s);
        assert(s.data != NULL);
        assert(s.len == 0);
        assert(s.cap >= 16);
        assert(strcmp(s.data, "") == 0);
        pt_str_free(&s);
        assert(s.data == NULL);
        assert(s.len == 0);
        assert(s.cap == 0);
        putchar('.');
}

static void test_nul_termination_mid_append(void) {
        pt_str *s = mk();
        pt_str_append(s, "abc");
        for (int i = 0; i < 10; i++) {
                pt_str_append_char(s, 'x');
                assert(s->data[s->len] == '\0');
        }
        fk(s);
        putchar('.');
}

static void test_many_empty_appends(void) {
        pt_str *s = mk();
        for (int i = 0; i < 5; i++) {
                int rc = pt_str_append(s, "");
                assert(rc == 0);
                assert(s->len == 0);
                assert(strcmp(s->data, "") == 0);
        }
        fk(s);
        putchar('.');
}

static void test_independence(void) {
        pt_str *a = pt_str_from("aaa");
        pt_str *b = pt_str_from("bbb");
        pt_str_append_char(a, 'X');
        pt_str_append_char(b, 'Y');
        assert(strcmp(a->data, "aaaX") == 0);
        assert(strcmp(b->data, "bbbY") == 0);
        fk(a);
        fk(b);
        putchar('.');
}

int main(void) {
        printf("Running pt_str tests...\n");
        test_new();
        test_from();
        test_append();
        test_resize();
        test_append_char();
        test_delete_char();
        test_free();

        /* edge‐case tests */
        test_delete_on_empty();
        test_from_empty();
        test_multiple_append_char();
        test_capacity_boundaries();
        test_double_free();
        test_append_after_delete();
        test_chain_append();
        test_init_on_stack();
        test_nul_termination_mid_append();
        test_many_empty_appends();
        test_independence();

        putchar('\n');
        printf("All pt_str tests passed.\n");
        return 0;
}

#endif /* PT_TEST */

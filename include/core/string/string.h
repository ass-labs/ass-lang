#ifndef __STRING_STRING_H__
#define __STRING_STRING_H__

#include "core/collection/vec.h"

#include <stdlib.h>
#include <string.h>

char *strndup(const char *s, size_t n);

declare_vec(char);

typedef vec(char) string_t;

void string_init(string_t *str);
void string_init_wcap(string_t *str, size_t cap);
void string_from(string_t *str, char const *s);
void string_repeat(string_t *str, char c, size_t n);
void string_free(string_t *str);

void string_push(string_t *str, char c);
void string_push_lit(string_t *str, char const *s);

void string_replacen(string_t *str, char const *old, char const *new, size_t n);
void string_replace(string_t *str, char const *old, char const *new);
void string_replace_all(string_t *str, char const *old, char const *new);

void string_assign(string_t *str, char const *s);

void string_rev(string_t *str);

declare_vec(string_t);

#endif

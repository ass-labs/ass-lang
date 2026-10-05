#ifndef __CORE_STRING_STRING_H__
#define __CORE_STRING_STRING_H__

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "core/collection/vec.h"

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

bool string_starts_with(string_t *str, char const *pref);
bool string_ends_with(string_t *str, char const *suf);

void string_assign(string_t *str, char const *s);

void string_rev(string_t *str);

declare_vec(string_t);

#endif

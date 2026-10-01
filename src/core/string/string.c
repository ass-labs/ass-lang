#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "core/collection/vec.h"
#include "core/string/string.h"
#include "utils/control.h"

char *strndup(const char *s, size_t n)
{
    size_t len = 0;
    while (len < n && s[len])
        len++;

    char *p = malloc(len + 1);
    if (!p)
        return NULL;

    memcpy(p, s, len);
    p[len] = '\0';

    return p;
}

define_vec(char);
define_vec(string_t);

void string_init(string_t *str)
{
    vec_new(char)(str, NULL, 16);
    str->data[0] = '\0';
}

void string_from(string_t *str, char const *s)
{
    size_t n = strlen(s);
    vec_new(char)(str, NULL, n + 1);

    memcpy(str->data, s, n);
    str->len = n;
    str->data[n] = '\0';
}

void string_repeat(string_t *str, char c, size_t n)
{
    vec_new_sized(char)(str, NULL, n);
    vec_for_each_mut(char, curr, (*str), { *curr = c; });
    str->data[n] = '\0';
}

void string_free(string_t *str) { vec_free(char)(str); }

void string_push(string_t *str, char c) { vec_push(char)(str, c); }
void string_push_lit(string_t *str, char const *s)
{
    size_t l_len = strlen(s);
    size_t n_len = str->len + l_len;

    while (n_len > str->cap)
        vec_update_cap(char)(str, str->cap * 2);

    memcpy(str->data + str->len, s, l_len);
    str->len += l_len;
}

void string_replacen(string_t *str, char const *old, char const *new, size_t n)
{
    size_t len = strlen(old), n_len = strlen(new), count = 0;

    for (size_t i = 0; i < (str->len - len) + 1 && count < n;)
    {
        if (memcmp(str->data + i, old, len) != 0)
        {
            ++i;
            continue;
        }

        size_t new_len = (str->len - len) + n_len;
        while (new_len + 1 > str->cap)
            vec_update_cap(char)(str, str->cap * 2);

        char *tailing = str->data + i + len;
        memmove(str->data + i + n_len, tailing, str->len - (i + len) + 1);
        memcpy(str->data + i, new, n_len);

        str->len = new_len;
        i += n_len;
        ++count;
    }
}

void string_replace(string_t *str, char const *old, char const *new)
{
    string_replacen(str, old, new, 1);
}

void string_replace_all(string_t *str, char const *old, char const *new)
{
    string_replacen(str, old, new, SIZE_MAX);
}

void string_assign(string_t *str, char const *s)
{
    size_t n = strlen(s);
    while (n > str->cap)
        vec_update_cap(char)(str, str->cap ? str->cap * 2 : 1);

    memcpy(str->data, s, n + 1);
    str->len = n;
    str->data[n] = '\0';
}

void string_rev(string_t *str)
{
    size_t n = str->len;
    for (size_t i = 0; i < n / 2; ++i)
    {
        size_t opp_idx = (n - 1) - i;
        char opp = str->data[opp_idx];

        str->data[opp_idx] = str->data[i];
        str->data[i] = opp;
    }
}

#ifndef __CORE_STRING_SLICE_H__
#define __CORE_STRING_SLICE_H__

#include "core/collection/vec.h"
#include "core/string/string.h"

#include "string.h"

typedef char_slice_t string_slice_t;
declare_vec(string_slice_t);

#define slice_fmt "%.*s"
#define use(slice) (int)slice.n, slice.data

string_slice_t as_slicen(char const *s, size_t n);
string_slice_t as_slice(char const *s);
vec(string_slice_t) string_slice_split(string_slice_t slice, char const *delim);

void string_slice_to_string(string_slice_t *slice, string_t *out);

void string_slice_trim_left(string_slice_t *slice);
void string_slice_trim_right(string_slice_t *slice);
void string_slice_trim(string_slice_t *slice);

bool string_slice_starts_with(string_slice_t *str, char const *pref);
bool string_slice_ends_with(string_slice_t *str, char const *suf);

#endif

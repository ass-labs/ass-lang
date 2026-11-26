#ifndef __SEMANTIC_TYPES_H__
#define __SEMANTIC_TYPES_H__

#include <stdbool.h>
#include <stdint.h>

#include "lexical/token.h"

/// Types the semantic analyzer gives to variables and expressions.
typedef enum : uint8_t
{
    /// Type of an expression that already has an error. Operations on it are
    /// not checked further, so one mistake is only reported once.
    STY_ERROR,
    /// Whole number (`wnum`)
    STY_WNUM,
    /// Decimal number (`dnum`)
    STY_DNUM,
    STY_BOOL,
    STY_CHAR,
    /// String literals. There is no string variable type yet (`str` is
    /// reserved), so strings can only be written.
    STY_STR,
} sem_ty_t;

/// Source name of a type, e.g. `"wnum"`.
char const *sem_ty_name(sem_ty_t ty);

/// Type named by a type keyword (`wnum`, `dnum`, `bool`, `char`).
sem_ty_t sem_ty_from_keyword(token_kind_t kind);

/// Type of a literal token.
sem_ty_t sem_ty_of_literal(token_kind_t kind);

bool sem_ty_is_numeric(sem_ty_t ty);

/// Whether a value of type `src` can be stored in a variable of type `dst`.
/// The only implicit conversion is `wnum` to `dnum`, which loses nothing.
bool sem_ty_assignable(sem_ty_t dst, sem_ty_t src);

#endif

#ifndef __AST_TY_H__
#define __AST_TY_H__

#include <stdint.h>

#include "lexical/token.h"

typedef struct
{
    union ty_data_t
    {
        /// A built-in type named by its keyword: `wnum`, `dnum`, `bool` or
        /// `char`.
        token_t nominal;
    } data;

    enum ty_kind_t : uint8_t
    {
        TY_NOMINAL,
    } kind;
} ty_t;

typedef enum ty_kind_t ty_kind_t;

ty_t ty_new_nominal(token_t name);

#endif

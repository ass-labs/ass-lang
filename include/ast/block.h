#ifndef __AST_BLOCK_H__
#define __AST_BLOCK_H__

#include "ast/stmt.h"

typedef struct block_t
{
    vec(stmt_t) stmts;
} block_t;

block_t *block_new(void);
/// Frees the statements of `block` and `block` itself. Accepts `NULL`.
void block_free(block_t *block);

#endif

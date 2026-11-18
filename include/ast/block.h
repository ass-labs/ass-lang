#ifndef __AST_BLOCK_H__
#define __AST_BLOCK_H__

#include "ast/stmt.h"

typedef struct
{
    vec(stmt_t) stmts;
} block_t;

void block_init(block_t *block);
void block_free(block_t *block);

#endif

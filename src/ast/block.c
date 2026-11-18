#include "ast/block.h"
#include "ast/stmt.h"

void block_init(block_t *block)
{
    vec_new(stmt_t)(&block->stmts, stmt_free, 8);
}

void block_free(block_t *block) { vec_free(stmt_t)(&block->stmts); }

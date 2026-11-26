#include "ast/block.h"
#include "ast/stmt.h"

block_t *block_new(void)
{
    block_t *block = malloc(sizeof(block_t));
    if (block == NULL)
        panic("failed to allocate block");

    vec_new(stmt_t)(&block->stmts, stmt_free, 8);
    return block;
}

void block_free(block_t *block)
{
    if (block == NULL)
        return;

    vec_free(stmt_t)(&block->stmts);
    free(block);
}

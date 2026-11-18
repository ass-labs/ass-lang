#include "ast/program.h"

void program_node_init(program_node_t *pnode)
{
    vec_new(stmt_t)(&pnode->stmts, stmt_free, 8);
}

void program_node_free(program_node_t *pnode)
{
    vec_free(stmt_t)(&pnode->stmts);
}

#ifndef __AST_PROGRAM_H__
#define __AST_PROGRAM_H__

#include "ast/stmt.h"
#include "core/collection/vec.h"

typedef struct
{
    vec(stmt_t) stmts;
} program_node_t;

void program_node_init(program_node_t *pnode);
void program_node_free(program_node_t *pnode);

#endif

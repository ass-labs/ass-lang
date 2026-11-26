#include "ast/stmt.h"
#include "ast/block.h"
#include "core/collection/vec.h"

define_vec(stmt_t);
define_vec(decl_item_t);

void decl_item_free(decl_item_t *item) { expr_free(item->val); }

void stmt_free(stmt_t *stmt)
{
    switch (stmt->kind)
    {
        case STMT_DECL:
            vec_free(decl_item_t)(&stmt->data.decl.items);
            break;
        case STMT_ASSIGN:
            expr_free(stmt->data.assign.val);
            break;
        case STMT_IF:
            expr_free(stmt->data._if.cond);
            block_free(stmt->data._if.consequent);
            block_free(stmt->data._if.alternate);
            break;
        case STMT_WHILE:
            expr_free(stmt->data._while.cond);
            block_free(stmt->data._while.body);
            break;
        case STMT_INPUT:
            vec_free(token_t)(&stmt->data.input.targets);
            break;
        case STMT_OUTPUT:
            vec_free(expr_ptr_t)(&stmt->data.output.vals);
            break;
    }
}

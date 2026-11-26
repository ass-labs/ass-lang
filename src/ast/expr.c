#include <stdlib.h>

#include "ast/expr.h"
#include "utils/control.h"

define_vec(expr_ptr_t);

static expr_t *expr_alloc(expr_kind_t kind)
{
    expr_t *expr = malloc(sizeof(expr_t));
    if (expr == NULL)
        panic("failed to allocate expr");

    expr->kind = kind;
    return expr;
}

expr_t *expr_new_literal(token_t literal)
{
    expr_t *expr = expr_alloc(EXPR_LITERAL);
    expr->data.literal = literal;
    return expr;
}

expr_t *expr_new_ident(token_t ident)
{
    expr_t *expr = expr_alloc(EXPR_IDENT);
    expr->data.ident = ident;
    return expr;
}

expr_t *expr_new_unary(token_t op, expr_t *operand)
{
    expr_t *expr = expr_alloc(EXPR_UNARY);
    expr->data.unary = (struct unary_expr_t){op, operand};
    return expr;
}

expr_t *expr_new_binary(token_t op, expr_t *lhs, expr_t *rhs)
{
    expr_t *expr = expr_alloc(EXPR_BINARY);
    expr->data.binary = (struct binary_expr_t){op, lhs, rhs};
    return expr;
}

void expr_free(expr_t *expr)
{
    if (expr == NULL)
        return;

    switch (expr->kind)
    {
        case EXPR_UNARY:
            expr_free(expr->data.unary.operand);
            break;
        case EXPR_BINARY:
            expr_free(expr->data.binary.lhs);
            expr_free(expr->data.binary.rhs);
            break;
        default:
            break;
    }

    free(expr);
}

void expr_ptr_free(expr_ptr_t *expr) { expr_free(*expr); }

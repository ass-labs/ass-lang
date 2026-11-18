#include "ast/expr.h"

void expr_init(expr_t *expr, expr_kind_t kind)
{
    expr->kind = kind;

    union expr_data_t *data = malloc(sizeof(union expr_data_t));
    if (data == NULL)
        panic("failed to allocate expr data buffer");

    expr->data = data;
}

void expr_free(expr_t *expr)
{
    // TODO: per kind clean-up
    free(expr->data);
}

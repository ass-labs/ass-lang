#include "ast/stmt.h"

void stmt_init(stmt_t *stmt, stmt_kind_t kind)
{
    stmt->kind = kind;

    union stmt_data_t *data = malloc(sizeof(union stmt_data_t));
    if (data == NULL)
        panic("failed to allocate stmt data buffer");

    stmt->data = data;
}

void stmt_free(stmt_t *stmt)
{
    // TODO: per kind clean-up
    free(stmt->data);
}

#ifndef __AST_EXPR_H__
#define __AST_EXPR_H__

#include "lexical/token.h"

typedef struct
{
    union expr_data_t
    {
        token_t *literal;
    } *data;

    enum expr_kind_t : uint8_t
    {
        EXPR_LITERAL,
    } kind;
} expr_t;

typedef enum expr_kind_t expr_kind_t;

void expr_init(expr_t *expr, expr_kind_t kind);
void expr_free(expr_t *expr);

#endif

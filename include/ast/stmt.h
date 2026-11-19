#ifndef __AST_STMT_H__
#define __AST_STMT_H__

#include "ast/expr.h"
#include "ast/ty.h"
#include "core/collection/vec.h"

struct block_t;

typedef struct
{
    union stmt_data_t
    {
        struct decl_stmt_t
        {
            ty_t *ty;
            expr_t *val;
        } decl;

        struct if_stmt_t
        {
            expr_t cond;
            struct block_t *consequent, *alternate;
        } _if;

        struct while_stmt_t
        {
            expr_t *cond;
            struct block_t *block;
        } _while;

        struct input_stmt_t
        {
            // TODO: input stmt data
        } input;

        struct output_stmt_t
        {
            // TODO: output stmt data
        } output;

        // TODO: expr
    } *data;

    enum stmt_kind_t : uint8_t
    {
        STMT_DECL,
        STMT_IF,
        STMT_WHILE,
        STMT_INPUT,
        STMT_OUTPUT,
        STMT_EXPR,
    } kind;
} stmt_t;

typedef enum stmt_kind_t stmt_kind_t;

declare_vec(stmt_t);

void stmt_init(stmt_t *stmt, stmt_kind_t kind);
void stmt_free(stmt_t *stmt);

#endif

#ifndef __AST_STMT_H__
#define __AST_STMT_H__

#include "ast/expr.h"
#include "ast/ty.h"
#include "core/collection/vec.h"

struct block_t;

/// One declared variable of a declaration, `x` or `x = <val>`.
typedef struct
{
    token_t name;
    /// `NULL` when the variable is declared without a value.
    expr_t *val;
} decl_item_t;

declare_vec(decl_item_t);

void decl_item_free(decl_item_t *item);

typedef struct
{
    union stmt_data_t
    {
        /// `[let] <ty> a = 1, b`
        struct decl_stmt_t
        {
            ty_t ty;
            vec(decl_item_t) items;
        } decl;

        /// `x = <val>`, `x += <val>`, ...
        struct assign_stmt_t
        {
            token_t target, op;
            expr_t *val;
        } assign;

        /// `if (<cond>) { ... } [else { ... }]`. An `else if` is an
        /// `alternate` block holding the nested `if` statement alone.
        struct if_stmt_t
        {
            expr_t *cond;
            struct block_t *consequent;
            /// `NULL` without an `else` branch.
            struct block_t *alternate;
        } _if;

        /// `while (<cond>) { ... }`
        struct while_stmt_t
        {
            expr_t *cond;
            struct block_t *body;
        } _while;

        /// `read(a, b)`
        struct input_stmt_t
        {
            vec(token_t) targets;
        } input;

        /// `write("x = ", x)`
        struct output_stmt_t
        {
            vec(expr_ptr_t) vals;
        } output;
    } data;

    enum stmt_kind_t : uint8_t
    {
        STMT_DECL,
        STMT_ASSIGN,
        STMT_IF,
        STMT_WHILE,
        STMT_INPUT,
        STMT_OUTPUT,
    } kind;
} stmt_t;

typedef enum stmt_kind_t stmt_kind_t;

declare_vec(stmt_t);

/// Frees the contents of `stmt` (statements are stored by value).
void stmt_free(stmt_t *stmt);

#endif

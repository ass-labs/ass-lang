#ifndef __AST_EXPR_H__
#define __AST_EXPR_H__

#include "core/collection/vec.h"
#include "lexical/token.h"

typedef struct expr_t expr_t;

struct expr_t
{
    union expr_data_t
    {
        /// `123`, `1.5`, `True`, `'a'`, `"text"`
        token_t literal;
        /// `x`
        token_t ident;
        /// `-x`, `NOT p`
        struct unary_expr_t
        {
            token_t op;
            expr_t *operand;
        } unary;
        /// `a + b`, `x < 3`, `p AND q`
        struct binary_expr_t
        {
            token_t op;
            expr_t *lhs, *rhs;
        } binary;
    } data;

    enum expr_kind_t : uint8_t
    {
        EXPR_LITERAL,
        EXPR_IDENT,
        EXPR_UNARY,
        EXPR_BINARY,
    } kind;
};

typedef enum expr_kind_t expr_kind_t;

typedef expr_t *expr_ptr_t;
declare_vec(expr_ptr_t);

expr_t *expr_new_literal(token_t literal);
expr_t *expr_new_ident(token_t ident);
expr_t *expr_new_unary(token_t op, expr_t *operand);
expr_t *expr_new_binary(token_t op, expr_t *lhs, expr_t *rhs);

/// Frees `expr` and its sub-expressions.
void expr_free(expr_t *expr);
/// `expr_free` for vectors of `expr_ptr_t`.
void expr_ptr_free(expr_ptr_t *expr);

#endif

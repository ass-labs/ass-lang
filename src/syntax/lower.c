#include "syntax/lower.h"
#include "ast/block.h"
#include "utils/guard.h"

static pt_node_t const *child(pt_node_t const *node, size_t i)
{
    return node->children.data[i];
}

/// Token of a nonterminal deriving a single token, e.g. `<add_op> => +`.
static token_t leaf_tok(pt_node_t const *node)
{
    return *child(node, 0)->tok;
}

static expr_t *lower_expr(pt_node_t const *node)
{
    size_t len = node->children.len;

    switch (node->sym)
    {
        case NT_BOOL_EXPR:
        case NT_EXPR:
        case NT_OUTPUT_ITEM:
            return lower_expr(child(node, 0));

        // <x> { <op> <x> }, folded to the left
        case NT_OR_EXPR:
        case NT_AND_EXPR:
        case NT_REL_EXPR:
        case NT_ADD_EXPR:
        case NT_MUL_EXPR:
        {
            expr_t *lhs = lower_expr(child(node, 0));
            for (size_t i = 1; i + 1 < len; i += 2)
                lhs = expr_new_binary(leaf_tok(child(node, i)), lhs,
                                      lower_expr(child(node, i + 1)));
            return lhs;
        }

        // [ <op> ] <x>
        case NT_NOT_EXPR:
        case NT_UNARY_EXPR:
            if (len == 1)
                return lower_expr(child(node, 0));
            return expr_new_unary(leaf_tok(child(node, 0)),
                                  lower_expr(child(node, 1)));

        // <primary> [ "^" <unary_expr> ]
        case NT_POW_EXPR:
            if (len == 1)
                return lower_expr(child(node, 0));
            return expr_new_binary(leaf_tok(child(node, 1)),
                                   lower_expr(child(node, 0)),
                                   lower_expr(child(node, 2)));

        // <ident> | <literal> | "(" <expr> ")"
        case NT_PRIMARY:
            if (len == 3)
                return lower_expr(child(node, 1));
            if (child(node, 0)->sym == NT_IDENT)
                return expr_new_ident(leaf_tok(child(node, 0)));
            return expr_new_literal(leaf_tok(child(child(node, 0), 0)));

        default:
            panic("unexpected %s in an expression", gsym_name(node->sym));
    }
}

static void lower_stmt(pt_node_t const *node, stmt_t *out);

// "{" { <stmt> } "}"
static block_t *lower_blk(pt_node_t const *node)
{
    block_t *block = block_new();
    for (size_t i = 1; i + 1 < node->children.len; ++i)
        lower_stmt(child(node, i), vec_expand(stmt_t)(&block->stmts));

    return block;
}

// "if" "(" <bool_expr> ")" <blk> [ "else" ( <cond_stmt> | <blk> ) ]
static void lower_if(pt_node_t const *node, stmt_t *out)
{
    out->kind = STMT_IF;
    out->data._if.cond = lower_expr(child(node, 2));
    out->data._if.consequent = lower_blk(child(node, 4));
    out->data._if.alternate = NULL;

    if (node->children.len < 7)
        return;

    pt_node_t const *alt = child(node, 6);
    if (alt->sym == NT_BLK)
    {
        out->data._if.alternate = lower_blk(alt);
        return;
    }

    block_t *block = block_new();
    lower_if(alt, vec_expand(stmt_t)(&block->stmts));
    out->data._if.alternate = block;
}

// [ "let" ] <type> <decl_assign> { "," <decl_assign> }
static void lower_decl(pt_node_t const *node, stmt_t *out)
{
    struct decl_stmt_t *decl = &out->data.decl;
    out->kind = STMT_DECL;

    size_t i = child(node, 0)->sym == TOK_NW_LET ? 1 : 0;
    decl->ty = ty_new_nominal(leaf_tok(child(node, i)));

    vec_new(decl_item_t)(&decl->items, decl_item_free, 4);
    for (i += 1; i < node->children.len; i += 2)
    {
        // <ident> [ "=" <expr> ]
        pt_node_t const *assign = child(node, i);
        decl_item_t item = {.name = leaf_tok(child(assign, 0)), .val = NULL};
        if (assign->children.len == 3)
            item.val = lower_expr(child(assign, 2));

        vec_push(decl_item_t)(&decl->items, item);
    }
}

static void lower_stmt(pt_node_t const *node, stmt_t *out)
{
    pt_node_t const *stmt = child(node, 0);

    switch (stmt->sym)
    {
        // "read" "(" <ident> { "," <ident> } ")"
        case NT_INPUT_STMT:
        {
            pt_node_t const *list = child(stmt, 2);
            out->kind = STMT_INPUT;
            vec_new(token_t)(&out->data.input.targets, NULL, 4);

            for (size_t i = 0; i < list->children.len; i += 2)
                vec_push(token_t)(&out->data.input.targets,
                                  leaf_tok(child(list, i)));
            break;
        }

        // "write" "(" <output_item> { "," <output_item> } ")"
        case NT_OUTPUT_STMT:
        {
            pt_node_t const *list = child(stmt, 2);
            out->kind = STMT_OUTPUT;
            vec_new(expr_ptr_t)(&out->data.output.vals, expr_ptr_free, 4);

            for (size_t i = 0; i < list->children.len; i += 2)
                vec_push(expr_ptr_t)(&out->data.output.vals,
                                     lower_expr(child(list, i)));
            break;
        }

        case NT_COND_STMT:
            lower_if(stmt, out);
            break;

        // "while" "(" <bool_expr> ")" <blk>
        case NT_ITER_STMT:
            out->kind = STMT_WHILE;
            out->data._while.cond = lower_expr(child(stmt, 2));
            out->data._while.body = lower_blk(child(stmt, 4));
            break;

        case NT_DECL_STMT:
            lower_decl(stmt, out);
            break;

        // <ident> <assign_op> <expr>
        case NT_ASSIGN_STMT:
            out->kind = STMT_ASSIGN;
            out->data.assign.target = leaf_tok(child(stmt, 0));
            out->data.assign.op = leaf_tok(child(stmt, 1));
            out->data.assign.val = lower_expr(child(stmt, 2));
            break;

        default:
            panic("unexpected %s in a statement", gsym_name(stmt->sym));
    }
}

void lower_program(pt_node_t const *root, program_node_t *program)
{
    pt_node_t const *list = child(root, 0);
    for (size_t i = 0; i < list->children.len; ++i)
        lower_stmt(child(list, i), vec_expand(stmt_t)(&program->stmts));
}

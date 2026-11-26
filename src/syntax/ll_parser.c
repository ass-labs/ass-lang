#include <setjmp.h>

#include "syntax/ll_parser.h"
#include "utils/log.h"

typedef struct
{
    parse_tree_t *tree;
    char const *file_path;
    token_t const *const *toks;
    size_t n, pos;
    /// Unwinds the recursion on the first syntax error.
    jmp_buf on_error;
} ll_parser_t;

static token_kind_t peek(ll_parser_t *p) { return p->toks[p->pos]->kind; }

_Noreturn static void syntax_error(ll_parser_t *p, char const *expected)
{
    token_t const *tok = p->toks[p->pos];

    if (tok->kind == TOK_EOF)
        eprintln("%s:%u:%u: syntax error (LL): expected %s, found end of file",
                 p->file_path, tok->row, tok->col, expected);
    else
        eprintln("%s:%u:%u: syntax error (LL): expected %s, found `" slice_fmt
                 "`",
                 p->file_path, tok->row, tok->col, expected, use(tok->lexeme));

    longjmp(p->on_error, 1);
}

/// Consumes the current token as a leaf of `parent`.
static void shift(ll_parser_t *p, pt_node_t *parent)
{
    pt_add_child(parent, pt_new_leaf(p->tree, p->toks[p->pos]));
    if (p->toks[p->pos]->kind != TOK_EOF)
        ++p->pos;
}

static void expect(ll_parser_t *p, pt_node_t *parent, token_kind_t kind)
{
    if (peek(p) != kind)
    {
        char expected[64];
        terminal_desc(expected, sizeof(expected), kind);
        syntax_error(p, expected);
    }

    shift(p, parent);
}

/// Parses a nonterminal deriving a single token, e.g. `<add_op> ::= "+"`.
static pt_node_t *token_rule(ll_parser_t *p, gsym_t nt)
{
    pt_node_t *node = pt_new_node(p->tree, nt);
    shift(p, node);
    return node;
}

static bool is_type(token_kind_t kind)
{
    return kind == TOK_RW_WNUM || kind == TOK_RW_DNUM || kind == TOK_KW_BOOL ||
           kind == TOK_KW_CHAR;
}

static bool is_stmt_start(token_kind_t kind)
{
    return kind == TOK_RW_READ || kind == TOK_RW_WRITE || kind == TOK_KW_IF ||
           kind == TOK_KW_WHILE || kind == TOK_NW_LET || kind == TOK_IDENT ||
           is_type(kind);
}

static bool is_assign_op(token_kind_t kind)
{
    return kind == TOK_EQ || kind == TOK_PLUS_EQ || kind == TOK_MINUS_EQ ||
           kind == TOK_STAR_EQ || kind == TOK_SLASH_EQ ||
           kind == TOK_SLASH_SLASH_EQ;
}

static bool is_relational_op(token_kind_t kind)
{
    return kind == TOK_GT || kind == TOK_LT || kind == TOK_GT_EQ ||
           kind == TOK_LT_EQ || kind == TOK_EQ_EQ || kind == TOK_BANG_EQ;
}

static bool is_mul_op(token_kind_t kind)
{
    return kind == TOK_STAR || kind == TOK_SLASH || kind == TOK_PERCENT ||
           kind == TOK_SLASH_SLASH;
}

static pt_node_t *stmt(ll_parser_t *p);
static pt_node_t *expr(ll_parser_t *p);
static pt_node_t *unary_expr(ll_parser_t *p);

// <ident> ::= IDENT
static pt_node_t *ident(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_IDENT);
    expect(p, node, TOK_IDENT);
    return node;
}

// <literal> ::= <wnum_lit> | <dnum_lit> | <bool_lit> | <char_lit>
//             | <string_lit>
static pt_node_t *literal(ll_parser_t *p)
{
    gsym_t lit;
    switch (peek(p))
    {
        case TOK_INT:
            lit = NT_WNUM_LIT;
            break;
        case TOK_FLOAT:
            lit = NT_DNUM_LIT;
            break;
        case TOK_KW_TRUE:
        case TOK_KW_FALSE:
            lit = NT_BOOL_LIT;
            break;
        case TOK_CHAR:
            lit = NT_CHAR_LIT;
            break;
        case TOK_STR:
            lit = NT_STRING_LIT;
            break;
        default:
            syntax_error(p, "a literal");
    }

    pt_node_t *node = pt_new_node(p->tree, NT_LITERAL);
    pt_add_child(node, token_rule(p, lit));
    return node;
}

// <primary> ::= <ident> | <literal> | "(" <expr> ")"
static pt_node_t *primary(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_PRIMARY);

    switch (peek(p))
    {
        case TOK_IDENT:
            pt_add_child(node, ident(p));
            break;
        case TOK_INT:
        case TOK_FLOAT:
        case TOK_KW_TRUE:
        case TOK_KW_FALSE:
        case TOK_CHAR:
        case TOK_STR:
            pt_add_child(node, literal(p));
            break;
        case TOK_LPAREN:
            shift(p, node);
            pt_add_child(node, expr(p));
            expect(p, node, TOK_RPAREN);
            break;
        default:
            syntax_error(p, "an expression");
    }

    return node;
}

// <pow_expr> ::= <primary> [ <pow_op> <unary_expr> ]
static pt_node_t *pow_expr(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_POW_EXPR);
    pt_add_child(node, primary(p));

    if (peek(p) == TOK_CARET)
    {
        pt_add_child(node, token_rule(p, NT_POW_OP));
        pt_add_child(node, unary_expr(p));
    }

    return node;
}

// <unary_expr> ::= <unary_op> <pow_expr> | <pow_expr>
static pt_node_t *unary_expr(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_UNARY_EXPR);

    if (peek(p) == TOK_PLUS || peek(p) == TOK_MINUS)
        pt_add_child(node, token_rule(p, NT_UNARY_OP));
    pt_add_child(node, pow_expr(p));

    return node;
}

// <mul_expr> ::= <unary_expr> { <mul_op> <unary_expr> }
static pt_node_t *mul_expr(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_MUL_EXPR);
    pt_add_child(node, unary_expr(p));

    while (is_mul_op(peek(p)))
    {
        pt_add_child(node, token_rule(p, NT_MUL_OP));
        pt_add_child(node, unary_expr(p));
    }

    return node;
}

// <add_expr> ::= <mul_expr> { <add_op> <mul_expr> }
static pt_node_t *add_expr(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_ADD_EXPR);
    pt_add_child(node, mul_expr(p));

    while (peek(p) == TOK_PLUS || peek(p) == TOK_MINUS)
    {
        pt_add_child(node, token_rule(p, NT_ADD_OP));
        pt_add_child(node, mul_expr(p));
    }

    return node;
}

// <rel_expr> ::= <add_expr> [ <relational_op> <add_expr> ]
static pt_node_t *rel_expr(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_REL_EXPR);
    pt_add_child(node, add_expr(p));

    if (is_relational_op(peek(p)))
    {
        pt_add_child(node, token_rule(p, NT_RELATIONAL_OP));
        pt_add_child(node, add_expr(p));
    }

    return node;
}

// <not_expr> ::= <not_op> <not_expr> | <rel_expr>
static pt_node_t *not_expr(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_NOT_EXPR);

    if (peek(p) == TOK_KW_NOT)
    {
        pt_add_child(node, token_rule(p, NT_NOT_OP));
        pt_add_child(node, not_expr(p));
    }
    else
        pt_add_child(node, rel_expr(p));

    return node;
}

// <and_expr> ::= <not_expr> { <and_op> <not_expr> }
static pt_node_t *and_expr(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_AND_EXPR);
    pt_add_child(node, not_expr(p));

    while (peek(p) == TOK_KW_AND)
    {
        pt_add_child(node, token_rule(p, NT_AND_OP));
        pt_add_child(node, not_expr(p));
    }

    return node;
}

// <or_expr> ::= <and_expr> { <or_op> <and_expr> }
static pt_node_t *or_expr(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_OR_EXPR);
    pt_add_child(node, and_expr(p));

    while (peek(p) == TOK_KW_OR)
    {
        pt_add_child(node, token_rule(p, NT_OR_OP));
        pt_add_child(node, and_expr(p));
    }

    return node;
}

// <expr> ::= <or_expr>
static pt_node_t *expr(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_EXPR);
    pt_add_child(node, or_expr(p));
    return node;
}

// <bool_expr> ::= <expr>
static pt_node_t *bool_expr(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_BOOL_EXPR);
    pt_add_child(node, expr(p));
    return node;
}

// <blk> ::= "{" { <stmt> } "}"
static pt_node_t *blk(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_BLK);
    expect(p, node, TOK_LBRACE);

    while (is_stmt_start(peek(p)))
        pt_add_child(node, stmt(p));

    if (peek(p) != TOK_RBRACE)
        syntax_error(p, "a statement or `}`");
    shift(p, node);

    return node;
}

// <input_stmt> ::= "read" "(" <input_list> ")"
// <input_list> ::= <ident> { "," <ident> }
static pt_node_t *input_stmt(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_INPUT_STMT);
    expect(p, node, TOK_RW_READ);
    expect(p, node, TOK_LPAREN);

    pt_node_t *list = pt_new_node(p->tree, NT_INPUT_LIST);
    pt_add_child(list, ident(p));
    while (peek(p) == TOK_COMMA)
    {
        shift(p, list);
        pt_add_child(list, ident(p));
    }
    pt_add_child(node, list);

    expect(p, node, TOK_RPAREN);
    return node;
}

// <output_item> ::= <expr>
static pt_node_t *output_item(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_OUTPUT_ITEM);
    pt_add_child(node, expr(p));
    return node;
}

// <output_stmt> ::= "write" "(" <output_list> ")"
// <output_list> ::= <output_item> { "," <output_item> }
static pt_node_t *output_stmt(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_OUTPUT_STMT);
    expect(p, node, TOK_RW_WRITE);
    expect(p, node, TOK_LPAREN);

    pt_node_t *list = pt_new_node(p->tree, NT_OUTPUT_LIST);
    pt_add_child(list, output_item(p));
    while (peek(p) == TOK_COMMA)
    {
        shift(p, list);
        pt_add_child(list, output_item(p));
    }
    pt_add_child(node, list);

    expect(p, node, TOK_RPAREN);
    return node;
}

// <cond_stmt> ::= "if" "(" <bool_expr> ")" <blk>
//                 [ "else" ( <cond_stmt> | <blk> ) ]
static pt_node_t *cond_stmt(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_COND_STMT);
    expect(p, node, TOK_KW_IF);
    expect(p, node, TOK_LPAREN);
    pt_add_child(node, bool_expr(p));
    expect(p, node, TOK_RPAREN);
    pt_add_child(node, blk(p));

    if (peek(p) != TOK_KW_ELSE)
        return node;

    shift(p, node);
    if (peek(p) == TOK_KW_IF)
        pt_add_child(node, cond_stmt(p));
    else if (peek(p) == TOK_LBRACE)
        pt_add_child(node, blk(p));
    else
        syntax_error(p, "`if` or `{`");

    return node;
}

// <iter_stmt> ::= "while" "(" <bool_expr> ")" <blk>
static pt_node_t *iter_stmt(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_ITER_STMT);
    expect(p, node, TOK_KW_WHILE);
    expect(p, node, TOK_LPAREN);
    pt_add_child(node, bool_expr(p));
    expect(p, node, TOK_RPAREN);
    pt_add_child(node, blk(p));
    return node;
}

// <decl_assign> ::= <ident> [ "=" <expr> ]
static pt_node_t *decl_assign(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_DECL_ASSIGN);
    pt_add_child(node, ident(p));

    if (peek(p) == TOK_EQ)
    {
        shift(p, node);
        pt_add_child(node, expr(p));
    }

    return node;
}

// <decl_stmt> ::= [ "let" ] <type> <decl_assign> { "," <decl_assign> }
static pt_node_t *decl_stmt(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_DECL_STMT);
    if (peek(p) == TOK_NW_LET)
        shift(p, node);

    if (!is_type(peek(p)))
        syntax_error(p, "a type (`wnum`, `dnum`, `bool` or `char`)");
    pt_add_child(node, token_rule(p, NT_TYPE));

    pt_add_child(node, decl_assign(p));
    while (peek(p) == TOK_COMMA)
    {
        shift(p, node);
        pt_add_child(node, decl_assign(p));
    }

    return node;
}

// <assign_stmt> ::= <ident> <assign_op> <expr>
static pt_node_t *assign_stmt(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_ASSIGN_STMT);
    pt_add_child(node, ident(p));

    if (!is_assign_op(peek(p)))
        syntax_error(p, "an assignment operator (`=`, `+=`, `-=`, `*=`, `/=` "
                        "or `//=`)");
    pt_add_child(node, token_rule(p, NT_ASSIGN_OP));
    pt_add_child(node, expr(p));

    return node;
}

// <stmt> ::= <input_stmt> | <output_stmt> | <cond_stmt> | <iter_stmt>
//          | <decl_stmt> | <assign_stmt>
static pt_node_t *stmt(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_STMT);
    token_kind_t kind = peek(p);

    if (kind == TOK_RW_READ)
        pt_add_child(node, input_stmt(p));
    else if (kind == TOK_RW_WRITE)
        pt_add_child(node, output_stmt(p));
    else if (kind == TOK_KW_IF)
        pt_add_child(node, cond_stmt(p));
    else if (kind == TOK_KW_WHILE)
        pt_add_child(node, iter_stmt(p));
    else if (kind == TOK_NW_LET || is_type(kind))
        pt_add_child(node, decl_stmt(p));
    else if (kind == TOK_IDENT)
        pt_add_child(node, assign_stmt(p));
    else
        syntax_error(p, "a statement");

    return node;
}

// <program>   ::= <stmt_list>
// <stmt_list> ::= { <stmt> }
static pt_node_t *program(ll_parser_t *p)
{
    pt_node_t *node = pt_new_node(p->tree, NT_PROGRAM);

    pt_node_t *list = pt_new_node(p->tree, NT_STMT_LIST);
    while (is_stmt_start(peek(p)))
        pt_add_child(list, stmt(p));
    pt_add_child(node, list);

    if (peek(p) != TOK_EOF)
        syntax_error(p, "a statement");

    return node;
}

bool ll_parse(parse_tree_t *tree, char const *file_path,
              token_t const *const *toks, size_t n)
{
    ll_parser_t p = {
        .tree = tree, .file_path = file_path, .toks = toks, .n = n, .pos = 0};

    if (setjmp(p.on_error) != 0)
        return false;

    tree->root = program(&p);
    return true;
}

#include <stdio.h>

#include "syntax/grammar.h"

#define P(lhs, ...)                                                            \
    {                                                                          \
        (lhs), sizeof((gsym_t[]){__VA_ARGS__}) / sizeof(gsym_t),               \
        {                                                                      \
            __VA_ARGS__                                                        \
        }                                                                      \
    }
#define P_EMPTY(lhs) {(lhs), 0, {0}}

production_t const PRODUCTIONS[] = {
    P(NT_START, NT_PROGRAM),

    // <program>   ::= <stmt_list>
    // <stmt_list> ::= { <stmt> }
    P(NT_PROGRAM, NT_STMT_LIST),
    P(NT_STMT_LIST, NT_STMTS),
    P_EMPTY(NT_STMTS),
    P(NT_STMTS, NT_STMTS, NT_STMT),

    // <stmt> ::= <input_stmt> | <output_stmt> | <cond_stmt> | <iter_stmt>
    //          | <decl_stmt> | <assign_stmt>
    P(NT_STMT, NT_INPUT_STMT),
    P(NT_STMT, NT_OUTPUT_STMT),
    P(NT_STMT, NT_COND_STMT),
    P(NT_STMT, NT_ITER_STMT),
    P(NT_STMT, NT_DECL_STMT),
    P(NT_STMT, NT_ASSIGN_STMT),

    // <blk> ::= "{" { <stmt> } "}"
    P(NT_BLK, TOK_LBRACE, NT_STMTS, TOK_RBRACE),

    // <input_stmt> ::= "read" "(" <input_list> ")"
    // <input_list> ::= <ident> { "," <ident> }
    P(NT_INPUT_STMT, TOK_RW_READ, TOK_LPAREN, NT_INPUT_LIST, TOK_RPAREN),
    P(NT_INPUT_LIST, NT_INPUT_ITEMS),
    P(NT_INPUT_ITEMS, NT_IDENT),
    P(NT_INPUT_ITEMS, NT_INPUT_ITEMS, TOK_COMMA, NT_IDENT),

    // <output_stmt> ::= "write" "(" <output_list> ")"
    // <output_list> ::= <output_item> { "," <output_item> }
    // <output_item> ::= <expr>
    P(NT_OUTPUT_STMT, TOK_RW_WRITE, TOK_LPAREN, NT_OUTPUT_LIST, TOK_RPAREN),
    P(NT_OUTPUT_LIST, NT_OUTPUT_ITEMS),
    P(NT_OUTPUT_ITEMS, NT_OUTPUT_ITEM),
    P(NT_OUTPUT_ITEMS, NT_OUTPUT_ITEMS, TOK_COMMA, NT_OUTPUT_ITEM),
    P(NT_OUTPUT_ITEM, NT_EXPR),

    // <cond_stmt> ::= "if" "(" <bool_expr> ")" <blk>
    //                 [ "else" ( <cond_stmt> | <blk> ) ]
    P(NT_COND_STMT, TOK_KW_IF, TOK_LPAREN, NT_BOOL_EXPR, TOK_RPAREN, NT_BLK),
    P(NT_COND_STMT, TOK_KW_IF, TOK_LPAREN, NT_BOOL_EXPR, TOK_RPAREN, NT_BLK,
      TOK_KW_ELSE, NT_COND_STMT),
    P(NT_COND_STMT, TOK_KW_IF, TOK_LPAREN, NT_BOOL_EXPR, TOK_RPAREN, NT_BLK,
      TOK_KW_ELSE, NT_BLK),

    // <iter_stmt> ::= "while" "(" <bool_expr> ")" <blk>
    P(NT_ITER_STMT, TOK_KW_WHILE, TOK_LPAREN, NT_BOOL_EXPR, TOK_RPAREN, NT_BLK),

    // <decl_stmt>   ::= [ "let" ] <type> <decl_assign> { "," <decl_assign> }
    // <decl_assign> ::= <ident> [ "=" <expr> ]
    P(NT_DECL_STMT, NT_TYPE, NT_DECL_LIST),
    P(NT_DECL_STMT, TOK_NW_LET, NT_TYPE, NT_DECL_LIST),
    P(NT_DECL_LIST, NT_DECL_ASSIGN),
    P(NT_DECL_LIST, NT_DECL_LIST, TOK_COMMA, NT_DECL_ASSIGN),
    P(NT_DECL_ASSIGN, NT_IDENT),
    P(NT_DECL_ASSIGN, NT_IDENT, TOK_EQ, NT_EXPR),

    // <assign_stmt> ::= <ident> <assign_op> <expr>
    // <assign_op>   ::= "=" | "+=" | "-=" | "*=" | "/=" | "//="
    P(NT_ASSIGN_STMT, NT_IDENT, NT_ASSIGN_OP, NT_EXPR),
    P(NT_ASSIGN_OP, TOK_EQ),
    P(NT_ASSIGN_OP, TOK_PLUS_EQ),
    P(NT_ASSIGN_OP, TOK_MINUS_EQ),
    P(NT_ASSIGN_OP, TOK_STAR_EQ),
    P(NT_ASSIGN_OP, TOK_SLASH_EQ),
    P(NT_ASSIGN_OP, TOK_SLASH_SLASH_EQ),

    // <bool_expr> ::= <expr>
    // <expr>      ::= <or_expr>
    P(NT_BOOL_EXPR, NT_EXPR),
    P(NT_EXPR, NT_OR_EXPR),

    // <or_expr>  ::= <and_expr> { <or_op> <and_expr> }
    P(NT_OR_EXPR, NT_OR_LIST),
    P(NT_OR_LIST, NT_AND_EXPR),
    P(NT_OR_LIST, NT_OR_LIST, NT_OR_OP, NT_AND_EXPR),

    // <and_expr> ::= <not_expr> { <and_op> <not_expr> }
    P(NT_AND_EXPR, NT_AND_LIST),
    P(NT_AND_LIST, NT_NOT_EXPR),
    P(NT_AND_LIST, NT_AND_LIST, NT_AND_OP, NT_NOT_EXPR),

    // <not_expr> ::= <not_op> <not_expr> | <rel_expr>
    P(NT_NOT_EXPR, NT_NOT_OP, NT_NOT_EXPR),
    P(NT_NOT_EXPR, NT_REL_EXPR),

    // <rel_expr> ::= <add_expr> [ <relational_op> <add_expr> ]
    P(NT_REL_EXPR, NT_ADD_EXPR),
    P(NT_REL_EXPR, NT_ADD_EXPR, NT_RELATIONAL_OP, NT_ADD_EXPR),

    // <add_expr> ::= <mul_expr> { <add_op> <mul_expr> }
    P(NT_ADD_EXPR, NT_ADD_LIST),
    P(NT_ADD_LIST, NT_MUL_EXPR),
    P(NT_ADD_LIST, NT_ADD_LIST, NT_ADD_OP, NT_MUL_EXPR),

    // <mul_expr> ::= <unary_expr> { <mul_op> <unary_expr> }
    P(NT_MUL_EXPR, NT_MUL_LIST),
    P(NT_MUL_LIST, NT_UNARY_EXPR),
    P(NT_MUL_LIST, NT_MUL_LIST, NT_MUL_OP, NT_UNARY_EXPR),

    // <unary_expr> ::= <unary_op> <pow_expr> | <pow_expr>
    P(NT_UNARY_EXPR, NT_UNARY_OP, NT_POW_EXPR),
    P(NT_UNARY_EXPR, NT_POW_EXPR),

    // <pow_expr> ::= <primary> [ <pow_op> <unary_expr> ]
    P(NT_POW_EXPR, NT_PRIMARY),
    P(NT_POW_EXPR, NT_PRIMARY, NT_POW_OP, NT_UNARY_EXPR),

    // <primary> ::= <ident> | <literal> | "(" <expr> ")"
    P(NT_PRIMARY, NT_IDENT),
    P(NT_PRIMARY, NT_LITERAL),
    P(NT_PRIMARY, TOK_LPAREN, NT_EXPR, TOK_RPAREN),

    P(NT_OR_OP, TOK_KW_OR),
    P(NT_AND_OP, TOK_KW_AND),
    P(NT_NOT_OP, TOK_KW_NOT),
    P(NT_RELATIONAL_OP, TOK_GT),
    P(NT_RELATIONAL_OP, TOK_LT),
    P(NT_RELATIONAL_OP, TOK_GT_EQ),
    P(NT_RELATIONAL_OP, TOK_LT_EQ),
    P(NT_RELATIONAL_OP, TOK_EQ_EQ),
    P(NT_RELATIONAL_OP, TOK_BANG_EQ),
    P(NT_ADD_OP, TOK_PLUS),
    P(NT_ADD_OP, TOK_MINUS),
    P(NT_MUL_OP, TOK_STAR),
    P(NT_MUL_OP, TOK_SLASH),
    P(NT_MUL_OP, TOK_PERCENT),
    P(NT_MUL_OP, TOK_SLASH_SLASH),
    P(NT_UNARY_OP, TOK_PLUS),
    P(NT_UNARY_OP, TOK_MINUS),
    P(NT_POW_OP, TOK_CARET),

    // <type> ::= "wnum" | "dnum" | "bool" | "char"
    P(NT_TYPE, TOK_RW_WNUM),
    P(NT_TYPE, TOK_RW_DNUM),
    P(NT_TYPE, TOK_KW_BOOL),
    P(NT_TYPE, TOK_KW_CHAR),

    // Identifiers and literals are recognized by the lexer, so the grammar
    // stops at their tokens.
    P(NT_IDENT, TOK_IDENT),

    // <literal> ::= <wnum_lit> | <dnum_lit> | <bool_lit> | <char_lit>
    //             | <string_lit>
    P(NT_LITERAL, NT_WNUM_LIT),
    P(NT_LITERAL, NT_DNUM_LIT),
    P(NT_LITERAL, NT_BOOL_LIT),
    P(NT_LITERAL, NT_CHAR_LIT),
    P(NT_LITERAL, NT_STRING_LIT),
    P(NT_WNUM_LIT, TOK_INT),
    P(NT_DNUM_LIT, TOK_FLOAT),
    P(NT_BOOL_LIT, TOK_KW_TRUE),
    P(NT_BOOL_LIT, TOK_KW_FALSE),
    P(NT_CHAR_LIT, TOK_CHAR),
    P(NT_STRING_LIT, TOK_STR),
};

size_t const N_PRODUCTIONS = sizeof(PRODUCTIONS) / sizeof(PRODUCTIONS[0]);

static char const *NT_NAMES[N_NONTERMINALS] = {
    [NT_PROGRAM - NT_BASE] = "<program>",
    [NT_STMT_LIST - NT_BASE] = "<stmt_list>",
    [NT_STMT - NT_BASE] = "<stmt>",
    [NT_BLK - NT_BASE] = "<blk>",
    [NT_INPUT_STMT - NT_BASE] = "<input_stmt>",
    [NT_INPUT_LIST - NT_BASE] = "<input_list>",
    [NT_OUTPUT_STMT - NT_BASE] = "<output_stmt>",
    [NT_OUTPUT_LIST - NT_BASE] = "<output_list>",
    [NT_OUTPUT_ITEM - NT_BASE] = "<output_item>",
    [NT_COND_STMT - NT_BASE] = "<cond_stmt>",
    [NT_ITER_STMT - NT_BASE] = "<iter_stmt>",
    [NT_DECL_STMT - NT_BASE] = "<decl_stmt>",
    [NT_DECL_ASSIGN - NT_BASE] = "<decl_assign>",
    [NT_ASSIGN_STMT - NT_BASE] = "<assign_stmt>",
    [NT_ASSIGN_OP - NT_BASE] = "<assign_op>",
    [NT_BOOL_EXPR - NT_BASE] = "<bool_expr>",
    [NT_EXPR - NT_BASE] = "<expr>",
    [NT_OR_EXPR - NT_BASE] = "<or_expr>",
    [NT_AND_EXPR - NT_BASE] = "<and_expr>",
    [NT_NOT_EXPR - NT_BASE] = "<not_expr>",
    [NT_REL_EXPR - NT_BASE] = "<rel_expr>",
    [NT_ADD_EXPR - NT_BASE] = "<add_expr>",
    [NT_MUL_EXPR - NT_BASE] = "<mul_expr>",
    [NT_UNARY_EXPR - NT_BASE] = "<unary_expr>",
    [NT_POW_EXPR - NT_BASE] = "<pow_expr>",
    [NT_PRIMARY - NT_BASE] = "<primary>",
    [NT_OR_OP - NT_BASE] = "<or_op>",
    [NT_AND_OP - NT_BASE] = "<and_op>",
    [NT_NOT_OP - NT_BASE] = "<not_op>",
    [NT_RELATIONAL_OP - NT_BASE] = "<relational_op>",
    [NT_ADD_OP - NT_BASE] = "<add_op>",
    [NT_MUL_OP - NT_BASE] = "<mul_op>",
    [NT_UNARY_OP - NT_BASE] = "<unary_op>",
    [NT_POW_OP - NT_BASE] = "<pow_op>",
    [NT_TYPE - NT_BASE] = "<type>",
    [NT_IDENT - NT_BASE] = "<ident>",
    [NT_LITERAL - NT_BASE] = "<literal>",
    [NT_WNUM_LIT - NT_BASE] = "<wnum_lit>",
    [NT_DNUM_LIT - NT_BASE] = "<dnum_lit>",
    [NT_BOOL_LIT - NT_BASE] = "<bool_lit>",
    [NT_CHAR_LIT - NT_BASE] = "<char_lit>",
    [NT_STRING_LIT - NT_BASE] = "<string_lit>",
    [NT_START - NT_BASE] = "<start>",
    [NT_STMTS - NT_BASE] = "<stmts>",
    [NT_INPUT_ITEMS - NT_BASE] = "<input_items>",
    [NT_OUTPUT_ITEMS - NT_BASE] = "<output_items>",
    [NT_DECL_LIST - NT_BASE] = "<decl_list>",
    [NT_OR_LIST - NT_BASE] = "<or_list>",
    [NT_AND_LIST - NT_BASE] = "<and_list>",
    [NT_ADD_LIST - NT_BASE] = "<add_list>",
    [NT_MUL_LIST - NT_BASE] = "<mul_list>",
};

char const *gsym_name(gsym_t sym)
{
    if (is_terminal(sym))
        return token_kind_str_repr((token_kind_t)sym);

    char const *name =
        sym < ____nt_end ? NT_NAMES[sym - NT_BASE] : NULL;
    return name != NULL ? name : "<?>";
}

void terminal_desc(char *buf, size_t n, gsym_t term)
{
    switch (term)
    {
        case TOK_IDENT:
            snprintf(buf, n, "an identifier");
            break;
        case TOK_INT:
            snprintf(buf, n, "an integer literal");
            break;
        case TOK_FLOAT:
            snprintf(buf, n, "a decimal literal");
            break;
        case TOK_CHAR:
            snprintf(buf, n, "a character literal");
            break;
        case TOK_STR:
            snprintf(buf, n, "a string literal");
            break;
        case TOK_EOF:
            snprintf(buf, n, "end of file");
            break;
        default:
            snprintf(buf, n, "`%s`", gsym_name(term));
    }
}

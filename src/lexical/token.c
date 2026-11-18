#include "lexical/token.h"

define_vec(token_t);

char const *TOKEN_KIND_STRS[____tok_count] = {
    [TOK_PLUS] = "+",          [TOK_MINUS] = "-",
    [TOK_STAR] = "*",          [TOK_SLASH] = "/",
    [TOK_PERCENT] = "%",       [TOK_SLASH_SLASH] = "//",

    [TOK_EQ] = "=",            [TOK_PLUS_EQ] = "+=",
    [TOK_MINUS_EQ] = "-=",     [TOK_STAR_EQ] = "*=",
    [TOK_SLASH_EQ] = "/=",     [TOK_SLASH_SLASH_EQ] = "//=",

    [TOK_EQ_EQ] = "==",        [TOK_BANG_EQ] = "!=",
    [TOK_LT] = "<",            [TOK_GT] = ">",
    [TOK_LT_EQ] = "<=",        [TOK_GT_EQ] = ">=",

    [TOK_KW_WNUM] = "wnum",    [TOK_KW_DNUM] = "dnum",
    [TOK_KW_CHAR] = "char",    [TOK_KW_BOOL] = "bool",
    [TOK_KW_IF] = "if",        [TOK_KW_ELSE] = "else",
    [TOK_KW_WHILE] = "while",  [TOK_KW_WRITE] = "write",
    [TOK_KW_READ] = "read",    [TOK_KW_AND] = "AND",
    [TOK_KW_OR] = "OR",        [TOK_KW_NOT] = "NOT",
    [TOK_KW_TRUE] = "True",    [TOK_KW_FALSE] = "False",

    [TOK_RW_WNUM8] = "wnum8",  [TOK_RW_WNUM16] = "wnum16",
    [TOK_RW_WNUM32] = "wnum32", [TOK_RW_WNUM64] = "wnum64",
    [TOK_RW_DNUM8] = "dnum8",  [TOK_RW_DNUM16] = "dnum16",
    [TOK_RW_DNUM32] = "dnum32", [TOK_RW_DNUM64] = "dnum64",
    [TOK_RW_AUTO] = "auto",    [TOK_RW_LET] = "let",
    [TOK_RW_STR] = "str",

    [TOK_DOT] = ".",           [TOK_COMMA] = ",",
    [TOK_SEMI_COLON] = ";",    [TOK_LBRACE] = "{",
    [TOK_RBRACE] = "}",        [TOK_LPAREN] = "(",
    [TOK_RPAREN] = ")",        [TOK_LBRACKET] = "[",
    [TOK_RBRACKET] = "]",
};

char const *TOKEN_KIND_NAMES[____tok_count] = {
    [TOK_IDENT] = "IDENT",
    [TOK_STR] = "STR",
    [TOK_CHAR] = "CHAR",
    [TOK_INT] = "INT",
    [TOK_FLOAT] = "FLOAT",
    [TOK_BOOL] = "BOOL",

    [TOK_PLUS] = "PLUS",
    [TOK_MINUS] = "MINUS",
    [TOK_STAR] = "STAR",
    [TOK_SLASH] = "SLASH",
    [TOK_PERCENT] = "PERCENT",
    [TOK_SLASH_SLASH] = "SLASH_SLASH",

    [TOK_EQ] = "EQ",
    [TOK_PLUS_EQ] = "PLUS_EQ",
    [TOK_MINUS_EQ] = "MINUS_EQ",
    [TOK_STAR_EQ] = "STAR_EQ",
    [TOK_SLASH_EQ] = "SLASH_EQ",
    [TOK_SLASH_SLASH_EQ] = "SLASH_SLASH_EQ",

    [TOK_EQ_EQ] = "EQ_EQ",
    [TOK_BANG_EQ] = "BANG_EQ",
    [TOK_LT] = "LT",
    [TOK_GT] = "GT",
    [TOK_LT_EQ] = "LT_EQ",
    [TOK_GT_EQ] = "GT_EQ",

    [TOK_KW_WNUM] = "KW_WNUM",
    [TOK_KW_DNUM] = "KW_DNUM",
    [TOK_KW_CHAR] = "KW_CHAR",
    [TOK_KW_BOOL] = "KW_BOOL",
    [TOK_KW_IF] = "KW_IF",
    [TOK_KW_ELSE] = "KW_ELSE",
    [TOK_KW_WHILE] = "KW_WHILE",
    [TOK_KW_WRITE] = "KW_WRITE",
    [TOK_KW_READ] = "KW_READ",
    [TOK_KW_AND] = "KW_AND",
    [TOK_KW_OR] = "KW_OR",
    [TOK_KW_NOT] = "KW_NOT",
    [TOK_KW_TRUE] = "KW_TRUE",
    [TOK_KW_FALSE] = "KW_FALSE",

    [TOK_RW_WNUM8] = "RW_WNUM8",
    [TOK_RW_WNUM16] = "RW_WNUM16",
    [TOK_RW_WNUM32] = "RW_WNUM32",
    [TOK_RW_WNUM64] = "RW_WNUM64",
    [TOK_RW_DNUM8] = "RW_DNUM8",
    [TOK_RW_DNUM16] = "RW_DNUM16",
    [TOK_RW_DNUM32] = "RW_DNUM32",
    [TOK_RW_DNUM64] = "RW_DNUM64",
    [TOK_RW_AUTO] = "RW_AUTO",
    [TOK_RW_LET] = "RW_LET",
    [TOK_RW_STR] = "RW_STR",

    [TOK_DOT] = "DOT",
    [TOK_COMMA] = "COMMA",
    [TOK_SEMI_COLON] = "SEMI_COLON",
    [TOK_LBRACE] = "LBRACE",
    [TOK_RBRACE] = "RBRACE",
    [TOK_LPAREN] = "LPAREN",
    [TOK_RPAREN] = "RPAREN",
    [TOK_LBRACKET] = "LBRACKET",
    [TOK_RBRACKET] = "RBRACKET",

    [TOK_LINE_COMMENT] = "LINE_COMMENT",
    [TOK_BLOCK_COMMENT] = "BLOCK_COMMENT",

    [TOK_WHITESPACE] = "WHITESPACE",
    [TOK_UNKNOWN] = "UNKNOWN",
};

char const *token_kind_str_repr(token_kind_t kind)
{
    char const *repr = kind < ____tok_count ? TOKEN_KIND_STRS[kind] : NULL;
    return repr != NULL ? repr : "";
}

char const *token_kind_name(token_kind_t kind)
{
    char const *name = kind < ____tok_count ? TOKEN_KIND_NAMES[kind] : NULL;
    return name != NULL ? name : "";
}

char const *token_kind_category(token_kind_t kind)
{
    switch (kind)
    {
    case TOK_IDENT:
        return "IDENTIFIER";
    case TOK_STR:
    case TOK_CHAR:
    case TOK_INT:
    case TOK_FLOAT:
    case TOK_BOOL:
    case TOK_KW_TRUE:
    case TOK_KW_FALSE:
        return "CONSTANT";
    case TOK_PLUS:
    case TOK_MINUS:
    case TOK_STAR:
    case TOK_SLASH:
    case TOK_PERCENT:
    case TOK_SLASH_SLASH:
        return "ARITH_OP";
    case TOK_EQ:
        return "ASSIGN_OP";
    case TOK_PLUS_EQ:
    case TOK_MINUS_EQ:
    case TOK_STAR_EQ:
    case TOK_SLASH_EQ:
    case TOK_SLASH_SLASH_EQ:
        return "RESERVED_OP";
    case TOK_EQ_EQ:
    case TOK_BANG_EQ:
    case TOK_LT:
    case TOK_GT:
    case TOK_LT_EQ:
    case TOK_GT_EQ:
        return "RELATIONAL_OP";
    case TOK_KW_AND:
    case TOK_KW_OR:
    case TOK_KW_NOT:
        return "LOGICAL_OP";
    case TOK_DOT:
    case TOK_COMMA:
    case TOK_SEMI_COLON:
        return "DELIMITER";
    case TOK_LBRACE:
    case TOK_RBRACE:
    case TOK_LPAREN:
    case TOK_RPAREN:
    case TOK_LBRACKET:
    case TOK_RBRACKET:
        return "BRACKET";
    case TOK_LINE_COMMENT:
    case TOK_BLOCK_COMMENT:
        return "COMMENT";
    case TOK_WHITESPACE:
        return "WHITESPACE";
    case TOK_UNKNOWN:
        return "INVALID";
    default:
        if (is_keyword(kind))
            return "KEYWORD";
        if (is_reserved(kind))
            return "RESERVED_WORD";
        return "";
    }
}

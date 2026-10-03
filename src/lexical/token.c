#include "lexical/token.h"

char const *TOKEN_KIND_STRS[] = {
    [TOK_KW_IF] = "if",     [TOK_KW_ELSE] = "else",   [TOK_KW_WHILE] = "while",
    [TOK_KW_AND] = "AND",   [TOK_KW_OR] = "OR",       [TOK_KW_NOT] = "NOT",
    [TOK_KW_TRUE] = "True", [TOK_KW_FALSE] = "False",
};

char const *token_kind_str_repr(token_kind_t kind)
{
    return kind >= ____tok_kw_start && kind <= ____tok_kw_end
               ? TOKEN_KIND_STRS[kind]
               : "";
}

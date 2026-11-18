#ifndef __LEXICAL_TOKEN_H__
#define __LEXICAL_TOKEN_H__

#include <stdint.h>

#include "core/collection/vec.h"
#include "core/string/slice.h"

typedef enum : uint8_t
{
    TOK_IDENT,
    TOK_STR,
    TOK_CHAR,
    TOK_INT,
    TOK_FLOAT,
    TOK_BOOL,

    // Arithmetic operators
    TOK_PLUS,        // +
    TOK_MINUS,       // -
    TOK_STAR,        // * - Asterisk
    TOK_SLASH,       // /
    TOK_PERCENT,     // %
    TOK_SLASH_SLASH, // // - Floor division

    // Assignment operators
    TOK_EQ, // =

    // Reserved compound assignment operators, lexed but not yet implemented
    TOK_PLUS_EQ,        // +=
    TOK_MINUS_EQ,       // -=
    TOK_STAR_EQ,        // *=
    TOK_SLASH_EQ,       // /=
    TOK_SLASH_SLASH_EQ, // //=

    // Relational operators
    TOK_EQ_EQ,   // ==
    TOK_BANG_EQ, // !=
    TOK_LT,      // <
    TOK_GT,      // >
    TOK_LT_EQ,   // <=
    TOK_GT_EQ,   // >=

    // Keywords
    ____tok_kw_start,

    TOK_KW_WNUM,
    TOK_KW_DNUM,
    TOK_KW_CHAR,
    TOK_KW_BOOL,

    TOK_KW_IF,
    TOK_KW_ELSE,
    TOK_KW_WHILE,

    TOK_KW_WRITE,
    TOK_KW_READ,

    TOK_KW_AND,
    TOK_KW_OR,
    TOK_KW_NOT,

    TOK_KW_TRUE,
    TOK_KW_FALSE,

    ____tok_kw_end,

    // Reserved words, lexed but not yet implemented
    ____tok_rw_start,

    TOK_RW_WNUM8,
    TOK_RW_WNUM16,
    TOK_RW_WNUM32,
    TOK_RW_WNUM64,
    TOK_RW_DNUM8,
    TOK_RW_DNUM16,
    TOK_RW_DNUM32,
    TOK_RW_DNUM64,

    TOK_RW_AUTO,
    TOK_RW_LET,
    TOK_RW_STR,

    ____tok_rw_end,

    // Delimeters
    TOK_DOT,        // .
    TOK_COMMA,      // ,
    TOK_SEMI_COLON, // ;

    TOK_LBRACE,   // {
    TOK_RBRACE,   // }
    TOK_LPAREN,   // (
    TOK_RPAREN,   // )
    TOK_LBRACKET, // [
    TOK_RBRACKET, // ]

    // Comments
    TOK_LINE_COMMENT,  // $$ ...
    TOK_BLOCK_COMMENT, // @$$ ... @$$

    // Special
    TOK_WHITESPACE, // Skipped by the lexer, never emitted
    TOK_UNKNOWN,    // Lexical error

    ____tok_count,
} token_kind_t;

#define is_keyword(kind) ((kind) > ____tok_kw_start && (kind) < ____tok_kw_end)
#define is_reserved(kind) ((kind) > ____tok_rw_start && (kind) < ____tok_rw_end)

extern char const *TOKEN_KIND_STRS[];
extern char const *TOKEN_KIND_NAMES[];

/// Source spelling of keywords, reserved words, operators and delimiters (e.g.
/// `"if"`, `"+"`), or `""` for token kinds without a fixed spelling.
char const *token_kind_str_repr(token_kind_t kind);
/// Display name of a token kind (e.g. `"KW_IF"`, `"PLUS"`).
char const *token_kind_name(token_kind_t kind);
/// Lexical category of a token kind (e.g. `"KEYWORD"`, `"ARITH_OP"`), as
/// shown in the symbol table.
char const *token_kind_category(token_kind_t kind);

typedef struct
{
    string_slice_t lexeme;
    token_kind_t kind;
    uint32_t row, col;
} token_t;

declare_vec(token_t);

#endif

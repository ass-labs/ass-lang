#ifndef __LEXICAL_DFA_H__
#define __LEXICAL_DFA_H__

#include <stdbool.h>
#include <stdint.h>

#include "lexical/token.h"

/// States of the lexer's DFA. Transitions are hard-coded as `switch`
/// statements in `dfa_step`, and accepting states are mapped to the token kind
/// they accept by `dfa_accepts`.
typedef enum : uint8_t
{
    /// Trap state: no token can be formed past this point.
    DFA_DEAD,
    DFA_START,

    // <ws>+
    DFA_WS,

    // Identifiers: [A-Za-z_][A-Za-z0-9_]*
    DFA_IDENT,

    // Keywords and reserved words, one state per prefix. Every prefix accepts
    // `TOK_IDENT` and leaves to `DFA_IDENT` on any other identifier character.
    DFA_W, DFA_WN, DFA_WNU, DFA_WNUM,            // wnum
    DFA_WNUM8, DFA_WNUM1, DFA_WNUM16,            // wnum8, wnum16
    DFA_WNUM3, DFA_WNUM32, DFA_WNUM6, DFA_WNUM64, // wnum32, wnum64
    DFA_WH, DFA_WHI, DFA_WHIL, DFA_WHILE,        // while
    DFA_WR, DFA_WRI, DFA_WRIT, DFA_WRITE,        // write
    DFA_D, DFA_DN, DFA_DNU, DFA_DNUM,            // dnum
    DFA_DNUM8, DFA_DNUM1, DFA_DNUM16,            // dnum8, dnum16
    DFA_DNUM3, DFA_DNUM32, DFA_DNUM6, DFA_DNUM64, // dnum32, dnum64
    DFA_C, DFA_CH, DFA_CHA, DFA_CHAR,            // char
    DFA_B, DFA_BO, DFA_BOO, DFA_BOOL,            // bool
    DFA_I, DFA_IF,                               // if
    DFA_E, DFA_EL, DFA_ELS, DFA_ELSE,            // else
    DFA_R, DFA_RE, DFA_REA, DFA_READ,            // read
    DFA_UA, DFA_UAN, DFA_UAND,                   // AND
    DFA_UO, DFA_UOR,                             // OR
    DFA_UN, DFA_UNO, DFA_UNOT,                   // NOT
    DFA_UT, DFA_UTR, DFA_UTRU, DFA_UTRUE,        // True
    DFA_UF, DFA_UFA, DFA_UFAL, DFA_UFALS, DFA_UFALSE, // False
    DFA_A, DFA_AU, DFA_AUT, DFA_AUTO,            // auto
    DFA_L, DFA_LE, DFA_LET,                      // let
    DFA_S, DFA_ST, DFA_STR,                      // str

    // Integers: <digit>+
    // Floats:   <digit>+ \. <digit>+
    DFA_INT,
    DFA_FRAC_SEP,
    DFA_FLOAT,
    /// Numbers running into identifier characters (`123variable`), lexed whole
    /// as a single error.
    DFA_BAD_IDENT,

    // Strings: \" (<char> | \\ <escape>)* \"
    DFA_STR_BODY,
    DFA_STR_ESC,
    DFA_STR_END,

    // Chars: \' (<char> | \\ <escape>) \'
    DFA_CHAR_OPEN,
    DFA_CHAR_ESC,
    DFA_CHAR_BODY,
    DFA_CHAR_END,

    // Operators and delimiters
    DFA_PLUS, DFA_PLUS_EQ,
    DFA_MINUS, DFA_MINUS_EQ,
    DFA_STAR, DFA_STAR_EQ,
    DFA_SLASH, DFA_SLASH_EQ, DFA_SLASH_SLASH, DFA_SLASH_SLASH_EQ,
    DFA_PERCENT,
    DFA_EQ, DFA_EQ_EQ,
    /// `!` is only a prefix of `!=` and does not accept on its own.
    DFA_BANG, DFA_BANG_EQ,
    DFA_LT, DFA_LT_EQ,
    DFA_GT, DFA_GT_EQ,
    DFA_DOT, DFA_COMMA, DFA_SEMI_COLON,
    DFA_LBRACE, DFA_RBRACE,
    DFA_LPAREN, DFA_RPAREN,
    DFA_LBRACKET, DFA_RBRACKET,

    // Single-line comments: \$\$ <char>*
    DFA_DOLLAR,
    DFA_LINE_COMMENT,

    // Multi-line comments: @\$\$ <any>* @\$\$
    DFA_BLK_OPEN_AT,
    DFA_BLK_OPEN_DOLLAR,
    DFA_BLK_BODY,
    DFA_BLK_CLOSE_AT,
    DFA_BLK_CLOSE_DOLLAR,
    DFA_BLK_END,
} dfa_state_t;

/// Returns the state reached from `state` on `input`, or `DFA_DEAD`.
dfa_state_t dfa_step(dfa_state_t state, uint8_t input);

/// Returns whether `state` is accepting, storing the token kind it accepts in
/// `kind` if so.
bool dfa_accepts(dfa_state_t state, token_kind_t *kind);

#endif

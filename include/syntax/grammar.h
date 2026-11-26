#ifndef __SYNTAX_GRAMMAR_H__
#define __SYNTAX_GRAMMAR_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lexical/token.h"

/// A grammar symbol. Terminals are the `token_kind_t`s (with `TOK_EOF` as the
/// end-of-input marker), nonterminals follow them starting at `NT_BASE`.
typedef uint16_t gsym_t;

#define NT_BASE ((gsym_t)____tok_count)
#define is_terminal(sym) ((sym) < NT_BASE)

typedef enum : uint16_t
{
    NT_PROGRAM = NT_BASE,
    NT_STMT_LIST,
    NT_STMT,
    NT_BLK,

    NT_INPUT_STMT,
    NT_INPUT_LIST,
    NT_OUTPUT_STMT,
    NT_OUTPUT_LIST,
    NT_OUTPUT_ITEM,
    NT_COND_STMT,
    NT_ITER_STMT,
    NT_DECL_STMT,
    NT_DECL_ASSIGN,
    NT_ASSIGN_STMT,
    NT_ASSIGN_OP,

    NT_BOOL_EXPR,
    NT_EXPR,
    NT_OR_EXPR,
    NT_AND_EXPR,
    NT_NOT_EXPR,
    NT_REL_EXPR,
    NT_ADD_EXPR,
    NT_MUL_EXPR,
    NT_UNARY_EXPR,
    NT_POW_EXPR,
    NT_PRIMARY,

    NT_OR_OP,
    NT_AND_OP,
    NT_NOT_OP,
    NT_RELATIONAL_OP,
    NT_ADD_OP,
    NT_MUL_OP,
    NT_UNARY_OP,
    NT_POW_OP,

    NT_TYPE,
    NT_IDENT,
    NT_LITERAL,
    NT_WNUM_LIT,
    NT_DNUM_LIT,
    NT_BOOL_LIT,
    NT_CHAR_LIT,
    NT_STRING_LIT,

    // Hidden nonterminals. They turn the EBNF repetitions (`{ ... }`) of the
    // grammar into left-recursive BNF rules for the LR parser, and are spliced
    // into their parent in the parse tree, so both parsers build the same flat
    // tree the EBNF grammar describes (e.g. `<add_expr> => <mul_expr> <add_op>
    // <mul_expr>`).
    ____nt_hidden_start,

    NT_START, // S' -> <program>, the augmented start symbol
    NT_STMTS,
    NT_INPUT_ITEMS,
    NT_OUTPUT_ITEMS,
    NT_DECL_LIST,
    NT_OR_LIST,
    NT_AND_LIST,
    NT_ADD_LIST,
    NT_MUL_LIST,

    ____nt_end,
} nonterminal_t;

#define N_TERMINALS ((size_t)NT_BASE)
#define N_NONTERMINALS ((size_t)(____nt_end - NT_BASE))
#define is_hidden(sym) ((sym) > ____nt_hidden_start && (sym) < ____nt_end)

#define PRODUCTION_MAX_LEN 7

typedef struct
{
    gsym_t lhs;
    uint8_t len;
    gsym_t rhs[PRODUCTION_MAX_LEN];
} production_t;

/// The BNF productions of the grammar. Production 0 is `S' -> <program>`.
extern production_t const PRODUCTIONS[];
extern size_t const N_PRODUCTIONS;

/// Name of a nonterminal (e.g. `"<add_expr>"`) or the spelling of a terminal.
char const *gsym_name(gsym_t sym);

/// Writes a human readable description of a terminal for error messages (e.g.
/// `an identifier`, `` `while` ``) into `buf`.
void terminal_desc(char *buf, size_t n, gsym_t term);

#endif

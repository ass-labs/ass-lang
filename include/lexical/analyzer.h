#ifndef __LEXICAL_ANALYZER_H__
#define __LEXICAL_ANALYZER_H__

#include <stdbool.h>
#include <stddef.h>

#include "automaton/state.h"
#include "core/string/slice.h"
#include "lexical/token.h"

typedef struct
{
    char const *file_path;
    string_slice_t src;

    state_reg_t state_reg;
    state_id_t root_id;

    /// Position of the next character to analyze
    size_t offset, row, col;

    vec(token_t) tokens;
    size_t err_count;
} lexer_t;

/// `file_path` is only used for error messages.
void lexer_new(lexer_t *lexer, char const *file_path, string_slice_t src);
void lexer_free(lexer_t *lexer);

/// Builds the lexer's DFA into `lexer->state_reg` and sets `lexer->root_id`.
///
/// Accepting states carry the `token_kind_t` they accept in `state_t.data`.
/// Inputs without a transition lead to `STATE_DEAD`.
void lexer_init_automaton(lexer_t *lexer);

bool lexer_eof(lexer_t *lexer);

/// Tokenizes the whole source into `lexer->tokens`, skipping whitespace.
/// Lexical errors are reported to stderr, counted in `lexer->err_count` and
/// emitted as `TOK_UNKNOWN` tokens so analysis can continue past them.
void lexer_analyze(lexer_t *lexer);

#endif

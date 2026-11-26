#ifndef __SYNTAX_LR_PARSER_H__
#define __SYNTAX_LR_PARSER_H__

#include <stdbool.h>
#include <stddef.h>

#include "syntax/parse_tree.h"

/// Bottom-up (SLR(1), shift-reduce) parser. Its reductions, read in reverse,
/// are a rightmost derivation of the program.
///
/// The parsing tables are built from `PRODUCTIONS` on the first call. A
/// shift-reduce or reduce-reduce conflict in the grammar is a bug and panics.
///
/// `toks` must end with a `TOK_EOF` token. On success the tree is stored in
/// `tree->root`, otherwise the first syntax error is reported to stderr and
/// `false` is returned.
bool lr_parse(parse_tree_t *tree, char const *file_path,
              token_t const *const *toks, size_t n);

/// Number of states of the SLR(1) automaton (builds the tables if needed).
size_t lr_state_count(void);

#endif

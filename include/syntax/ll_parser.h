#ifndef __SYNTAX_LL_PARSER_H__
#define __SYNTAX_LL_PARSER_H__

#include <stdbool.h>
#include <stddef.h>

#include "syntax/parse_tree.h"

/// Top-down (LL(1), recursive descent) parser. It reads the tokens left to
/// right and expands the leftmost nonterminal first, so the tree it builds is
/// naturally read as a leftmost derivation.
///
/// `toks` must end with a `TOK_EOF` token. On success the tree is stored in
/// `tree->root`, otherwise the first syntax error is reported to stderr and
/// `false` is returned.
bool ll_parse(parse_tree_t *tree, char const *file_path,
              token_t const *const *toks, size_t n);

#endif

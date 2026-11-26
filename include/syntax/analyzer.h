#ifndef __SYNTAX_ANALYZER_H__
#define __SYNTAX_ANALYZER_H__

#include <stdbool.h>

#include "ast/program.h"
#include "core/collection/vec.h"
#include "lexical/analyzer.h"
#include "syntax/parse_tree.h"

typedef token_t const *token_ref_t;
declare_vec(token_ref_t);

typedef struct
{
    char const *file_path;
    /// The tokens of the lexer without comments, ended by `eof`.
    vec(token_ref_t) toks;
    token_t eof;

    /// Parse trees of the LL and LR parsers. `root` is `NULL` when the parser
    /// is disabled in `CONFIG.p_conf` or failed.
    parse_tree_t ll_tree, lr_tree;
} parser_t;

/// `lexer` must outlive the parser, the parse trees point to its tokens.
void parser_new(parser_t *parser, lexer_t const *lexer);
void parser_free(parser_t *parser);

/// Runs the parsers enabled in `CONFIG.p_conf` and builds the AST of the
/// program into `program` (which must be initialized). Syntax errors are
/// reported to stderr and make it return `false`.
bool parser_parse(parser_t *parser, program_node_t *program);

#endif

#ifndef __SYNTAX_LOWER_H__
#define __SYNTAX_LOWER_H__

#include "ast/program.h"
#include "syntax/parse_tree.h"

/// Builds the AST of a successfully parsed program from its parse tree.
/// `program` must be initialized.
void lower_program(pt_node_t const *root, program_node_t *program);

#endif

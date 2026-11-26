#ifndef __AST_PRINT_H__
#define __AST_PRINT_H__

#include <stdio.h>

#include "ast/program.h"

/// Writes the AST as an indented S-expression, e.g.
///
///   (program
///     (decl wnum
///       (x (+ 1 2)))
///     (while (< x 3)
///       (block
///         (assign x += 1))))
void ast_write(FILE *out, program_node_t const *program);

#endif

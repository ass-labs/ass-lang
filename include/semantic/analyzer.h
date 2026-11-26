#ifndef __SEMANTIC_ANALYZER_H__
#define __SEMANTIC_ANALYZER_H__

#include <stdbool.h>
#include <stdint.h>

#include "ast/program.h"
#include "core/collection/vec.h"
#include "semantic/diagnostic.h"
#include "semantic/types.h"

/// A declared variable, alive while its scope is being analyzed.
typedef struct
{
    /// The name token at the declaration.
    token_t name;
    sem_ty_t ty;
    /// Nesting depth of the declaring scope, 0 for the program.
    uint32_t depth;

    /// Holds a value on every path reaching the current point.
    bool def_init;
    /// Holds a value on at least one path reaching the current point.
    bool maybe_init;

    /// Its value is read somewhere.
    bool read;
    /// It is assigned (`=`, `+=`, `read(...)`) after its declaration.
    bool assigned;
    /// A use before initialization was reported, so it isn't reported again.
    bool uninit_reported;
} symbol_t;

declare_vec(symbol_t);

typedef struct
{
    /// Scope stack: the variables of the innermost scope are last.
    vec(symbol_t) syms;
    diag_list_t diags;
    uint32_t depth;
    /// While positive, diagnostics are not reported (used for the first pass
    /// over loop bodies).
    uint32_t silent;
} sema_t;

void sema_new(sema_t *sema, char const *file_path);
void sema_free(sema_t *sema);

/// Checks `program` and prints its errors and warnings to stderr, sorted by
/// position. Returns `false` if there were errors (warnings don't count).
///
/// Checks:
/// - types of expressions, assignments, conditions and declarations
/// - undeclared variables and redefinitions in the same scope
/// - variables shadowing a variable of an enclosing scope (warning)
/// - unused variables, and variables assigned but never read (warnings)
/// - variables read before being initialized: an error when no path
///   initializes them, a warning when only some paths do
bool sema_analyze(sema_t *sema, program_node_t const *program);

#endif

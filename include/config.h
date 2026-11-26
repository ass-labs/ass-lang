#ifndef __CONFIG_H__
#define __CONFIG_H__

/// Main internal configuration file of ASS-lang

#include <stdbool.h>
#include <stdint.h>

/// Output configuration, a bit flag: combine options with `|` (e.g.
/// `OCONF_GEN_SYM_TABLE | OCONF_LOG_SYM_TABLE`) and test them with `&`.
typedef enum : uint8_t
{
    OCONF_NONE = 0,
    /// Writes the symbol table of each source file to `<file>.sym`
    OCONF_GEN_SYM_TABLE = 1 << 0,
    /// Prints the symbol table of each source file to stdout
    OCONF_LOG_SYM_TABLE = 1 << 1,
    /// Writes the leftmost derivation of each source file (from the LL
    /// parser) to `<file>.leftmost` and the rightmost derivation (from the LR
    /// parser) to `<file>.rightmost`
    OCONF_GEN_DERIVATIONS = 1 << 2,
    /// Writes the AST of each source file to `<file>.ast`
    OCONF_GEN_AST = 1 << 3,
} out_config_t;

/// Syntax analyzers to run, a bit flag. When both run, their parse trees are
/// checked to be identical.
typedef enum : uint8_t
{
    PCONF_NONE = 0,
    /// Top-down parser: expands the leftmost nonterminal first (left to
    /// right), producing a leftmost derivation
    PCONF_LL = 1 << 0,
    /// Bottom-up parser: its reductions form a rightmost derivation (right to
    /// left) in reverse
    PCONF_LR = 1 << 1,
} parser_config_t;

typedef struct
{
    char const *file_ext;
    out_config_t out_conf;
    parser_config_t p_conf;
    bool debug;
} config_t;

extern config_t CONFIG;

#endif

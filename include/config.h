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
} out_config_t;

typedef struct
{
    char const *file_ext;
    out_config_t out_conf;
    bool debug;
} config_t;

extern config_t CONFIG;

#endif

#include "config.h"

char const *FILE_EXT = ".ass";

out_config_t OUT_CONFIG =
    /// Generates the symbol table file per file
    OCONF_GEN_SYM_TABLE
    /// Logs the symbol table per file in the stdout
    // | OCONF_LOG_SYM_TABLE
    ;

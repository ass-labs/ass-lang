#include "config.h"

config_t CONFIG = (config_t){
    .file_ext = ".ass",
    .out_conf = OCONF_GEN_SYM_TABLE      /// Generates the symbol table file
                | OCONF_GEN_DERIVATIONS  /// Generates the derivation files
                | OCONF_GEN_AST          /// Generates the AST file
                                         // | OCONF_LOG_SYM_TABLE ///  Logs the
                                         // symbol table per file in the stdout
    ,
    .p_conf = PCONF_LL   /// Top-down parser, leftmost derivation
              | PCONF_LR /// Bottom-up parser, rightmost derivation
    ,
    .debug = true,
};

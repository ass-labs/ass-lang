#ifndef __CONFIG_H__
#define __CONFIG_H__

/// Main internal configuration file of ASS-lang

#include <stdbool.h>
#include <stdint.h>

extern char const *FILE_EXT;

typedef enum : uint8_t
{
    OCONF_GEN_SYM_TABLE = 1 << 0,
    OCONF_LOG_SYM_TABLE = 1 << 1,
} out_config_t;

extern out_config_t OUT_CONFIG;

#endif

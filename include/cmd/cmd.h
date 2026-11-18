#ifndef __CMD_CMD_H__
#define __CMD_CMD_H__

#include "core/string/slice.h"

typedef struct
{
    string_slice_t name, sh;
} cmd_opt_t;

typedef struct cmd_t
{
    string_slice_t name;
    uint32_t (*handler)(struct cmd_t const *cmd,
                        slice(string_slice_t) arguments);
} cmd_t;

#endif

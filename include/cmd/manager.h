#ifndef __CMD_MANAGER_H__
#define __CMD_MANAGER_H__

#include "cmd/cmd.h"
#include "core/collection/hash_map.h"

declare_hm(string_slice_t, cmd_t);

typedef struct
{
    hash_map(string_slice_t, cmd_t) cmds;
} cmd_manager_t;

void cmd_manager_new(cmd_manager_t *cmdm);

void cmd_manager_free(cmd_manager_t *cmdm);

void cmd_manager_init_cmds(cmd_manager_t *cmdm);

#endif

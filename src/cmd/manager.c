#include "cmd/manager.h"
#include "utils/guard.h"

define_hm(string_slice_t, cmd_t);

void cmd_manager_new(cmd_manager_t *cmdm)
{
    hm_new(string_slice_t, cmd_t)(&cmdm->cmds, hasher(string_slice_t),
                                  eq(string_slice_t), NULL, NULL);

    cmd_manager_init_cmds(cmdm);
}

void cmd_manager_free(cmd_manager_t *cmdm)
{
    hm_free(string_slice_t, cmd_t)(&cmdm->cmds);
}

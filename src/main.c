#include <stdlib.h>
#include <string.h>

#include "cmd/manager.h"
#include "core/collection/hash_map.h"
#include "core/collection/slice.h"
#include "core/string/slice.h"
#include "utils/drop.h"
#include "utils/log.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        eprintln("usage: ass <command> [arguments]\n\n"
                 "commands:\n"
                 "  run <file>  execute an ASS-lang source file");
        return EXIT_FAILURE;
    }

    string_slice_t command = as_slice(argv[1]);

    drop(vec_free(string_slice_t)) vec(string_slice_t) arguments;
    vec_new(string_slice_t)(&arguments, NULL, (size_t)argc);
    for (size_t i = 2; i < (size_t)argc; ++i)
        *vec_expand(string_slice_t)(&arguments) = as_slice(argv[i]);

    drop(cmd_manager_free) cmd_manager_t cmdm;
    cmd_manager_new(&cmdm);

    cmd_t const *cmd = hm_get(string_slice_t, cmd_t)(&cmdm.cmds, &command);
    if (cmd == NULL)
    {
        eprintln("error: unknown command '" slice_fmt "'", use(command));
        return EXIT_FAILURE;
    }

    return cmd->handler(cmd,
                        view(string_slice_t)(&arguments, 0, arguments.len));
}

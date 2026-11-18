#include "cmd/manager.h"
#include "program/program.h"
#include "utils/drop.h"
#include "utils/guard.h"

struct cmd_data
{
    char const *name;
    uint32_t (*handler)(cmd_t const *, slice(string_slice_t));
};

uint32_t run_cmd_handler(cmd_t const *cmd, slice(string_slice_t) arguments);
uint32_t tests_cmd_handler(cmd_t const *cmd, slice(string_slice_t) arguments);

void cmd_manager_init_cmds(cmd_manager_t *cmdm)
{
    struct cmd_data cmds[] = {{.name = "run", .handler = run_cmd_handler},
                              {.name = "tests", .handler = tests_cmd_handler}};

    for (size_t i = 0; i < (sizeof(cmds) / sizeof(cmds[0])); ++i)
    {
        struct cmd_data cmd = cmds[i];

        string_slice_t name = as_slice(cmd.name);

        hm_insert(string_slice_t, cmd_t)(&cmdm->cmds, name,
                                         (cmd_t){name, cmd.handler});
    }
}

uint32_t run_cmd_handler(cmd_t const *cmd, slice(string_slice_t) arguments)
{
    (void)cmd;
    if (arguments.n < 1)
    {
        eprintln("error: missing file to run, usage: ass run <file>");
        return EXIT_FAILURE;
    }

    string_slice_t const *file = arguments.data;

    drop(program_free) program_t program;
    program_new(&program, file->data);

    return program_run(&program) ? EXIT_SUCCESS : EXIT_FAILURE;
}

uint32_t tests_cmd_handler(cmd_t const *cmd, slice(string_slice_t) arguments)
{
    (void)cmd, (void)arguments;
    todo("implement `tests` command");
}

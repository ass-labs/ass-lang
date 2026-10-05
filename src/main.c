#include "program/program.h"
#include "utils/drop.h"

int main(int argc, char **argv)
{
    (void)argc, (void)argv;

    char const *fpath = "tests/lexical/keywords.ass";

    drop(program_free) program_t program;
    program_new(&program, fpath);

    program_run(&program);

    return 0;
}

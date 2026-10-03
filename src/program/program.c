#include <stdio.h>
#include <string.h>

#include "config.h"
#include "program/program.h"
#include "utils/control.h"
#include "utils/guard.h"

void program_new(program_t *program, char const *fpath)
{
    program->file_path = as_slice(fpath);
}

void program_free(program_t *program) { string_free(&program->src); }

void program_read_source(program_t *program)
{
    size_t fext_len = strlen(CONFIG.file_ext);
    if (program->file_path.n >= fext_len &&
        strncmp(program->file_path.data + (program->file_path.n - fext_len),
                CONFIG.file_ext, fext_len) != 0)
    {
        panic("entry point file provided must be an ASS-lang file and ends "
              "with `.ass`");
    }

    FILE *file = fopen(program->file_path.data, "r");
    if (file == NULL)
        panic("failed to read file: %s", program->file_path);

    fseek(file, 0, SEEK_END);
    size_t fsize = ftell(file);
    fseek(file, 0, 0);

    string_init_wcap(&program->src, fsize);
    size_t read_size = fread(program->src.data, sizeof(char), fsize, file);

    if (fsize != read_size)
        panic("failed to completely read file: %s", program->file_path);

    fclose(file);
}

void program_run(program_t *program)
{
    program_read_source(program);

    todo("lexical analyzer");

    todo("syntax analyzer");
}

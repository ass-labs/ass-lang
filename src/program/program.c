#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "ast/print.h"
#include "ast/program.h"
#include "config.h"
#include "core/string/slice.h"
#include "core/string/string.h"
#include "lexical/analyzer.h"
#include "program/program.h"
#include "semantic/analyzer.h"
#include "syntax/analyzer.h"
#include "utils/control.h"
#include "utils/drop.h"
#include "utils/guard.h"

#ifdef _WIN32
#include <windows.h>
#define PATH_SEPARATOR '\\'
#else
#include <unistd.h>
#define PATH_SEPARATOR '/'
#endif

#define SYM_TABLE_EXT ".sym"
#define LEFTMOST_EXT ".leftmost"
#define RIGHTMOST_EXT ".rightmost"
#define AST_EXT ".ast"

static bool is_regular_file(const char *path)
{
    struct stat buf;
    int status = stat(path, &buf);

    if (status != 0)
        return false;

    return S_ISREG(buf.st_mode);
}

static void canonicalize_path(string_t *abs_path, const char *path)
{
#ifdef _WIN32
    DWORD size = GetFullPathNameA(path, 0, NULL, NULL);
    if (size == 0)
        panic("cannot find absolute path of '%s'", path);

    drop(string_free) string_t resolved;
    string_init_wcap(&resolved, size);

    DWORD len = GetFullPathNameA(path, size, resolved.data, NULL);
    if (len == 0 || len >= size)
        panic("cannot find absolute path of '%s'", path);

    for (char *p = resolved.data; *p; p++)
        if (*p == '\\')
            *p = '/';

    string_assign(abs_path, resolved.data);
#else
    char resolved[PATH_MAX];
    if (realpath(path, resolved) == NULL)
        panic("cannot find absolute path of '%s'", path);

    string_assign(abs_path, resolved);
#endif
}

void program_new(program_t *program, char const *fpath)
{
    string_init(&program->src);
    string_from(&program->file_path, fpath);
}

void program_free(program_t *program)
{
    string_free(&program->src);
    string_free(&program->file_path);
}

void program_read_source(program_t *program)
{
    if (!is_regular_file(program->file_path.data))
        panic("invalid file path provided");

    if (!string_ends_with(&program->file_path, CONFIG.file_ext))
    {
        panic("entry point file provided must be an ASS-lang file and ends "
              "with `%s`",
              CONFIG.file_ext);
    }

    canonicalize_path(&program->file_path, program->file_path.data);

    // Binary mode, so that `\r\n` is kept as-is and the size matches `ftell`.
    FILE *file = fopen(program->file_path.data, "rb");
    if (file == NULL)
        panic("failed to read file: %s", program->file_path.data);

    fseek(file, 0, SEEK_END);
    size_t fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    vec_update_cap(char)(&program->src, fsize + 1);
    size_t read_size = fread(program->src.data, sizeof(char), fsize, file);
    fclose(file);

    if (fsize != read_size)
        panic("failed to completely read file: %s", program->file_path.data);

    program->src.len = read_size;
    program->src.data[read_size] = '\0';
}

/// Writes `lexeme` on a single line, escaping line endings and tabs (e.g. of
/// multi-line comments) so every token stays on its own row.
static void write_lexeme(FILE *out, string_slice_t lexeme)
{
    for (size_t i = 0; i < lexeme.n; ++i)
    {
        switch (lexeme.data[i])
        {
            case '\n':
                fputs("\\n", out);
                break;
            case '\r':
                fputs("\\r", out);
                break;
            case '\t':
                fputs("\\t", out);
                break;
            default:
                fputc(lexeme.data[i], out);
        }
    }
}

static void write_sym_table(FILE *out, lexer_t const *lexer)
{
    fprintf(out, "%-10s %-14s %-16s %s\n", "ROW:COL", "CATEGORY", "KIND",
            "LEXEME");
    vec_for_each(token_t, tok, lexer->tokens, {
        char pos[24];
        snprintf(pos, sizeof(pos), "%u:%u", tok->row, tok->col);
        fprintf(out, "%-10s %-14s %-16s ", pos, token_kind_category(tok->kind),
                token_kind_name(tok->kind));
        write_lexeme(out, tok->lexeme);
        fputc('\n', out);
    });
}

/// Opens `<source file><ext>` for writing, storing its path in `out_path`
/// (which must be initialized).
static FILE *open_output(program_t const *program, char const *ext,
                         string_t *out_path)
{
    size_t size = program->file_path.len + strlen(ext) + 1;

    string_init_wcap(out_path, size);
    snprintf(out_path->data, size, "%s%s", program->file_path.data, ext);
    out_path->len = size - 1;

    FILE *out = fopen(out_path->data, "w");
    if (out == NULL)
        panic("failed to write output file: %s", out_path->data);

    return out;
}

static void program_output_sym_table(program_t *program, lexer_t const *lexer)
{
    if (CONFIG.out_conf & OCONF_GEN_SYM_TABLE)
    {
        drop(string_free) string_t out_path;
        FILE *out = open_output(program, SYM_TABLE_EXT, &out_path);

        write_sym_table(out, lexer);
        fclose(out);

        dprintln("symbol table written to '%s'", out_path.data);
    }

    if (CONFIG.out_conf & OCONF_LOG_SYM_TABLE)
        write_sym_table(stdout, lexer);
}

static void program_output_derivation(program_t *program, pt_node_t const *root,
                                      bool rightmost)
{
    if (root == NULL || !(CONFIG.out_conf & OCONF_GEN_DERIVATIONS))
        return;

    drop(string_free) string_t out_path;
    FILE *out = open_output(
        program, rightmost ? RIGHTMOST_EXT : LEFTMOST_EXT, &out_path);

    pt_write_derivation(out, root, rightmost);
    fclose(out);

    dprintln("%s derivation written to '%s'",
             rightmost ? "rightmost" : "leftmost", out_path.data);
}

static void program_output_ast(program_t *program, program_node_t const *ast)
{
    if (!(CONFIG.out_conf & OCONF_GEN_AST))
        return;

    drop(string_free) string_t out_path;
    FILE *out = open_output(program, AST_EXT, &out_path);

    ast_write(out, ast);
    fclose(out);

    dprintln("AST written to '%s'", out_path.data);
}

bool program_run(program_t *program)
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    program_read_source(program);
    dprintln("running file '%s'", program->file_path.data);

    // Lexical Analysis
    drop(lexer_free) lexer_t lexer;
    lexer_new(&lexer, program->file_path.data,
              as_slicen(program->src.data, program->src.len));
    lexer_analyze(&lexer);

    program_output_sym_table(program, &lexer);

    // Syntactic Analysis, skipped after lexical errors since the parsers
    // would only report them again as unexpected tokens.
    bool ok = lexer.err_count == 0;
    if (ok)
    {
        drop(parser_free) parser_t parser;
        parser_new(&parser, &lexer);

        drop(program_node_free) program_node_t ast;
        program_node_init(&ast);

        ok = parser_parse(&parser, &ast);
        program_output_derivation(program, parser.ll_tree.root, false);
        program_output_derivation(program, parser.lr_tree.root, true);
        if (ok)
            program_output_ast(program, &ast);

        // Semantic Analysis, on the AST of a program without syntax errors.
        if (ok)
        {
            drop(sema_free) sema_t sema;
            sema_new(&sema, program->file_path.data);
            ok = sema_analyze(&sema, &ast);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000L +
                      (end.tv_nsec - start.tv_nsec),
         elapsed_mus = elapsed_ns / 1000;
    dprintln("time elapsed: %ld ns, %ld μs", elapsed_ns, elapsed_mus);

    return ok;
}

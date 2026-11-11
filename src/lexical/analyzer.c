#include <stdint.h>
#include <string.h>

#include "lexical/analyzer.h"
#include "lexical/dfa.h"
#include "lexical/token.h"
#include "utils/log.h"

void lexer_new(lexer_t *lexer, char const *file_path, string_slice_t src)
{
    lexer->file_path = file_path;
    lexer->src = src;
    lexer->offset = 0;
    lexer->row = lexer->col = 1;
    lexer->err_count = 0;

    vec_new(token_t)(&lexer->tokens, NULL, 64);
}

void lexer_free(lexer_t *lexer) { vec_free(token_t)(&lexer->tokens); }

bool lexer_eof(lexer_t *lexer) { return lexer->offset >= lexer->src.n; }

/// Maximal munch: runs the DFA from the current offset until it reaches the
/// dead state and returns the length of the longest accepted prefix (0 if
/// none), storing the kind of that prefix in `kind`.
static size_t lexer_munch(lexer_t *lexer, token_kind_t *kind)
{
    size_t acc_len = 0;
    dfa_state_t state = DFA_START;

    for (size_t i = lexer->offset; i < lexer->src.n; ++i)
    {
        state = dfa_step(state, (uint8_t)lexer->src.data[i]);
        if (state == DFA_DEAD)
            break;

        if (dfa_accepts(state, kind))
            acc_len = (i - lexer->offset) + 1;
    }

    return acc_len;
}

/// Picks how much input to skip after a lexical error at the current offset,
/// so that a bad literal is reported once instead of cascading into more
/// errors.
static size_t lexer_recover(lexer_t *lexer, char const **reason)
{
    size_t pos = lexer->offset;

    // An unterminated multi-line comment swallows the rest of the file.
    if (lexer->src.n - pos >= 3 &&
        strncmp(lexer->src.data + pos, "@$$", 3) == 0)
    {
        *reason = "unterminated multi-line comment";
        return lexer->src.n - pos;
    }

    char quote = lexer->src.data[pos];
    if (quote != '"' && quote != '\'')
    {
        *reason = "unrecognized character";
        return 1;
    }

    *reason = quote == '"' ? "unterminated or malformed string literal"
                           : "unterminated or malformed character literal";

    // Skip to the closing quote on the same line, or to the end of the line.
    size_t i = pos + 1;
    for (; i < lexer->src.n && lexer->src.data[i] != '\n'; ++i)
    {
        char c = lexer->src.data[i];
        if (c == '\\' && i + 1 < lexer->src.n && lexer->src.data[i + 1] != '\n')
            ++i;
        else if (c == quote)
            return (i - pos) + 1;
    }

    // Leave the `\r` of a `\r\n` line ending to the whitespace automaton.
    if (i > pos + 1 && lexer->src.data[i - 1] == '\r')
        --i;

    return i - pos;
}

/// Moves the cursor `len` characters forward, keeping `row` and `col` in sync.
static void lexer_advance(lexer_t *lexer, size_t len)
{
    for (size_t i = 0; i < len; ++i, ++lexer->offset)
    {
        if (lexer->src.data[lexer->offset] == '\n')
            ++lexer->row, lexer->col = 1;
        else
            ++lexer->col;
    }
}

void lexer_analyze(lexer_t *lexer)
{
    while (!lexer_eof(lexer))
    {
        token_t tok = {.row = lexer->row, .col = lexer->col};
        size_t len = lexer_munch(lexer, &tok.kind);

        if (len == 0 || tok.kind == TOK_UNKNOWN)
        {
            char const *reason = "identifiers cannot start with a digit";
            if (len == 0)
                len = lexer_recover(lexer, &reason);
            tok.kind = TOK_UNKNOWN;

            // Only quote the first line of multi-line lexemes.
            char const *start = lexer->src.data + lexer->offset;
            char const *nl = memchr(start, '\n', len);
            size_t shown = nl != NULL ? (size_t)(nl - start) : len;

            eprintln("%s:%zu:%zu: error: %s: `%.*s`", lexer->file_path,
                     lexer->row, lexer->col, reason, (int)shown, start);
            ++lexer->err_count;
        }

        tok.lexeme = as_slicen(lexer->src.data + lexer->offset, len);
        if (tok.kind != TOK_WHITESPACE)
            vec_push(token_t)(&lexer->tokens, tok);

        lexer_advance(lexer, len);
    }
}

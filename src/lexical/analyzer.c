#include <stdint.h>

#include "automaton/state.h"
#include "lexical/analyzer.h"
#include "lexical/token.h"
#include "utils/log.h"

/// Characters allowed after a backslash in string and char literals.
#define ESCAPE_CHARS "ntr0\\'\""
#define WHITESPACE_CHARS " \t\r\n"

void lexer_new(lexer_t *lexer, char const *file_path, string_slice_t src)
{
    lexer->file_path = file_path;
    lexer->src = src;
    lexer->offset = 0;
    lexer->row = lexer->col = 1;
    lexer->err_count = 0;

    state_reg_init(&lexer->state_reg);
    lexer_init_automaton(lexer);

    vec_new(token_t)(&lexer->tokens, NULL, 64);
}

void lexer_free(lexer_t *lexer)
{
    vec_free(token_t)(&lexer->tokens);
    state_reg_free(&lexer->state_reg);
}

static void insert_ident_start_chars(state_reg_t *reg, state_id_t id,
                                     state_id_t dest_id)
{
    state_reg_insert_range(reg, id, 'a', 'z', dest_id);
    state_reg_insert_range(reg, id, 'A', 'Z', dest_id);
    state_reg_insert(reg, id, '_', dest_id);
}

static void insert_ident_chars(state_reg_t *reg, state_id_t id,
                               state_id_t dest_id)
{
    insert_ident_start_chars(reg, id, dest_id);
    state_reg_insert_range(reg, id, '0', '9', dest_id);
}

// <ws>+
static void init_whitespace_automaton(state_reg_t *reg, state_id_t root_id)
{
    state_id_t ws_id = state_reg_register(reg, STF_ACCEPTING, TOK_WHITESPACE);

    state_reg_insert_set(reg, root_id, WHITESPACE_CHARS, ws_id);
    state_reg_insert_set(reg, ws_id, WHITESPACE_CHARS, ws_id);
}

// Identifiers: [A-Za-z_][A-Za-z0-9_]*
//
// Keywords are paths branching off the identifier automaton. Every state along
// a keyword's path accepts `TOK_IDENT` (`wh` is a valid identifier) and leaves
// to the generic identifier state on any other identifier character, while the
// last state of the path accepts the keyword itself:
//
//   () -i-> ((i)) -f-> ((if: KW_IF))
//              |             |
//              | [^f]        | [A-Za-z0-9_]
//              v             v
//         ((ident: IDENT)) <-+
//              ^   |
//              +---+ [A-Za-z0-9_]
static void init_ident_kw_automaton(state_reg_t *reg, state_id_t root_id)
{
    state_id_t ident_id = state_reg_register(reg, STF_ACCEPTING, TOK_IDENT);

    insert_ident_start_chars(reg, root_id, ident_id);
    insert_ident_chars(reg, ident_id, ident_id);

    for (size_t kind = ____tok_kw_start + 1; kind < ____tok_kw_end; ++kind)
    {
        char const *kw = token_kind_str_repr((token_kind_t)kind);

        state_id_t curr_id = root_id;
        for (char const *c = kw; *c != '\0'; ++c)
        {
            state_id_t next_id =
                state_step(state_reg_get(reg, curr_id), (uint8_t)*c);

            // Branch off the generic identifier state unless another keyword
            // already created this prefix.
            if (next_id == ident_id || next_id == STATE_DEAD)
            {
                next_id = state_reg_register(reg, STF_ACCEPTING, TOK_IDENT);
                insert_ident_chars(reg, next_id, ident_id);
                state_reg_insert(reg, curr_id, (uint8_t)*c, next_id);
            }

            curr_id = next_id;
        }

        state_reg_get_mut(reg, curr_id)->data = (uint8_t)kind;
    }
}

// Integers: <digit>+
// Floats:   <digit>+ \. <digit>+
static void init_num_lit_automaton(state_reg_t *reg, state_id_t root_id)
{
    // <digit>+
    state_id_t digit_id = state_reg_register(reg, STF_ACCEPTING, TOK_INT);
    state_reg_insert_range(reg, root_id, '0', '9', digit_id);
    state_reg_insert_range(reg, digit_id, '0', '9', digit_id);

    // \.
    state_id_t frac_sep_id = state_reg_register(reg, STF_NONE, 0);
    state_reg_insert(reg, digit_id, '.', frac_sep_id);

    // <frac_digit>+
    state_id_t frac_digit_id =
        state_reg_register(reg, STF_ACCEPTING, TOK_FLOAT);
    state_reg_insert_range(reg, frac_sep_id, '0', '9', frac_digit_id);
    state_reg_insert_range(reg, frac_digit_id, '0', '9', frac_digit_id);
}

// Strings: \" (<char> | \\ <escape>)* \"
// where <char> is any byte except \", \\ and a newline.
static void init_str_lit_automaton(state_reg_t *reg, state_id_t root_id)
{
    state_id_t body_id = state_reg_register(reg, STF_NONE, 0);
    state_id_t esc_id = state_reg_register(reg, STF_NONE, 0);
    state_id_t end_id = state_reg_register(reg, STF_ACCEPTING, TOK_STR);

    state_reg_insert(reg, root_id, '"', body_id);

    state_reg_insert_range(reg, body_id, 0, UINT8_MAX, body_id);
    state_reg_insert(reg, body_id, '\n', STATE_DEAD);
    state_reg_insert(reg, body_id, '\\', esc_id);
    state_reg_insert(reg, body_id, '"', end_id);

    state_reg_insert_set(reg, esc_id, ESCAPE_CHARS, body_id);
}

// Chars: \' (<char> | \\ <escape>) \'
// where <char> is any byte except \', \\ and a newline.
static void init_char_lit_automaton(state_reg_t *reg, state_id_t root_id)
{
    state_id_t open_id = state_reg_register(reg, STF_NONE, 0);
    state_id_t esc_id = state_reg_register(reg, STF_NONE, 0);
    state_id_t char_id = state_reg_register(reg, STF_NONE, 0);
    state_id_t end_id = state_reg_register(reg, STF_ACCEPTING, TOK_CHAR);

    state_reg_insert(reg, root_id, '\'', open_id);

    state_reg_insert_range(reg, open_id, 0, UINT8_MAX, char_id);
    state_reg_insert(reg, open_id, '\n', STATE_DEAD);
    state_reg_insert(reg, open_id, '\'', STATE_DEAD);
    state_reg_insert(reg, open_id, '\\', esc_id);

    state_reg_insert_set(reg, esc_id, ESCAPE_CHARS, char_id);
    state_reg_insert(reg, char_id, '\'', end_id);
}

// Operators and delimiters, all single characters.
static void init_symbol_automaton(state_reg_t *reg, state_id_t root_id)
{
    token_kind_t const kinds[] = {
        TOK_PLUS,     TOK_MINUS,    TOK_STAR,   // TOK_SLASH removed - handled below
        TOK_PERCENT,  TOK_DOT,      TOK_COMMA,  TOK_SEMI_COLON,
        TOK_LBRACE,   TOK_RBRACE,   TOK_LPAREN, TOK_RPAREN,
        TOK_LBRACKET, TOK_RBRACKET,
    };

    for (size_t i = 0; i < (sizeof(kinds) / sizeof(kinds[0])); ++i)
    {
        char const *repr = token_kind_str_repr(kinds[i]);
        state_id_t sym_id = state_reg_register(reg, STF_ACCEPTING, kinds[i]);
        state_reg_insert(reg, root_id, (uint8_t)repr[0], sym_id);
    }

    // Handle / and // as two-character operators
    state_id_t slash_id = state_reg_register(reg, STF_ACCEPTING, TOK_SLASH);
    state_reg_insert(reg, root_id, '/', slash_id);

    state_id_t slash_slash_id = state_reg_register(reg, STF_ACCEPTING, TOK_SLASH_SLASH);
    state_reg_insert(reg, slash_id, '/', slash_slash_id);
}

void lexer_init_automaton(lexer_t *lexer)
{
    state_reg_t *reg = &lexer->state_reg;
    state_id_t root_id = state_reg_register(reg, STF_ROOT, 0);

    init_whitespace_automaton(reg, root_id);
    init_ident_kw_automaton(reg, root_id);
    init_num_lit_automaton(reg, root_id);
    init_str_lit_automaton(reg, root_id);
    init_char_lit_automaton(reg, root_id);
    init_symbol_automaton(reg, root_id);

    lexer->root_id = root_id;
}

bool lexer_eof(lexer_t *lexer) { return lexer->offset >= lexer->src.n; }

/// Maximal munch: runs the DFA from the current offset until it reaches the
/// dead state and returns the length of the longest accepted prefix (0 if
/// none), storing the kind of that prefix in `kind`.
static size_t lexer_munch(lexer_t *lexer, token_kind_t *kind)
{
    size_t acc_len = 0;
    state_id_t curr_id = lexer->root_id;

    for (size_t i = lexer->offset; i < lexer->src.n; ++i)
    {
        state_t const *curr = state_reg_get(&lexer->state_reg, curr_id);
        curr_id = state_step(curr, (uint8_t)lexer->src.data[i]);
        if (curr_id == STATE_DEAD)
            break;

        state_t const *next = state_reg_get(&lexer->state_reg, curr_id);
        if (is_accepting(*next))
        {
            acc_len = (i - lexer->offset) + 1;
            *kind = (token_kind_t)next->data;
        }
    }

    return acc_len;
}

/// Picks how much input to skip after a lexical error at the current offset,
/// so that a bad literal is reported once instead of cascading into more
/// errors.
static size_t lexer_recover(lexer_t *lexer, char const **reason)
{
    size_t pos = lexer->offset;
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

        if (len == 0)
        {
            char const *reason;
            len = lexer_recover(lexer, &reason);
            tok.kind = TOK_UNKNOWN;

            eprintln("%s:%zu:%zu: error: %s: `%.*s`", lexer->file_path,
                     lexer->row, lexer->col, reason, (int)len,
                     lexer->src.data + lexer->offset);
            ++lexer->err_count;
        }

        tok.lexeme = as_slicen(lexer->src.data + lexer->offset, len);
        if (tok.kind != TOK_WHITESPACE)
            vec_push(token_t)(&lexer->tokens, tok);

        lexer_advance(lexer, len);
    }
}

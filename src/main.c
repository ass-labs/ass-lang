#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>

#include "core/collection/vec.h"
#include "core/string/slice.h"
#include "core/string/string.h"
#include "utils/control.h"
#include "utils/drop.h"
#include "utils/guard.h"
#include "utils/log.h"

typedef enum : uint8_t
{
    ____tok_kw_start,

    TOK_KW_IF,
    TOK_KW_ELSE,
    TOK_KW_WHILE,

    TOK_KW_AND,
    TOK_KW_OR,
    TOK_KW_NOT,

    TOK_KW_TRUE,
    TOK_KW_FALSE,

    ____tok_kw_end,
} token_kind_t;

static char const *TOK_KW_STRS[] = {
    [TOK_KW_IF] = "if",     [TOK_KW_ELSE] = "else",   [TOK_KW_WHILE] = "while",
    [TOK_KW_AND] = "AND",   [TOK_KW_OR] = "OR",       [TOK_KW_NOT] = "NOT",
    [TOK_KW_TRUE] = "True", [TOK_KW_FALSE] = "False",
};

static char const *token_kind_str_repr(token_kind_t kind)
{
    return kind <= ____tok_kw_start || kind >= ____tok_kw_end
               ? ""
               : TOK_KW_STRS[kind];
}

typedef struct
{
    char const *lexeme;
    token_kind_t data;
} token_t;

typedef struct state_t
{
    struct state_t *tr_table;
    uint8_t data;
    char input;
} state_t;

#define is_accepting(data) (data != UINT8_MAX)

static size_t const LOWER_ALPHA_OFS = 0, UPPER_ALPHA_OFS = ('z' - 'a') + 1,
                    NUM_OFS = UPPER_ALPHA_OFS + (('Z' - 'A') + 1),
                    SYM_OFS = NUM_OFS + (('9' - '0') + 1);

static size_t const TR_TABLE_SIZE = SYM_OFS + 1 /* NOTE: Number of symbols */;

size_t idx_of(char c)
{
    return isalpha(c)   ? islower(c) ? LOWER_ALPHA_OFS + (c - 'a')
                                     : UPPER_ALPHA_OFS + (c - 'A')
             : isalnum(c) ? NUM_OFS + (c - '0')
           : c == '_'   ? SYM_OFS
                        : (panic("invalid character"), SIZE_MAX);
}

bool is_occupied(state_t *state) { return state->input != '\0'; }

void state_init(state_t *state)
{
    *state = (state_t){0};
    state->tr_table = calloc(sizeof(state_t), TR_TABLE_SIZE);
}

void state_free(state_t *state)
{
    if (state->tr_table == NULL)
        return;

    for (size_t i = 0; i < TR_TABLE_SIZE; ++i)
    {
        state_t *c_state = state->tr_table + i;
        if (is_occupied(c_state))
            state_free(c_state);
    }

    free(state->tr_table);
}

void state_insert(state_t *root, string_slice_t str, uint8_t data)
{
    state_t *curr = root;
    for (size_t i = 0; i < str.n; ++i)
    {
        char c = str.data[i];
        state_t *c_state = curr->tr_table + idx_of(c);

        if (is_occupied(c_state))
            state_insert(c_state, as_slicen(str.data + i + 1, str.n - (i + 1)),
                         data);
        else
        {
            state_init(c_state);
            c_state->input = c;
        }

        curr = c_state;
    }

    curr->data = data;
}

uint8_t state_get(state_t *root, string_slice_t str)
{
    state_t *curr = root;
    for (size_t i = 0; i < str.n; ++i)
    {
        char c = str.data[i];
        state_t *c_state = curr->tr_table + idx_of(c);

        if (!is_occupied(c_state))
            return c_state->data;

        curr = c_state;
    }

    return curr->data;
}

void state_print(state_t *state, uint32_t indent)
{
    print("%*s(%c) ", (int)indent * 2, "", state->input);
    if (is_accepting(state->data))
        print("%s", token_kind_str_repr(state->data));

    println("");

    for (size_t i = 0; i < TR_TABLE_SIZE; ++i)
    {
        state_t *c_state = state->tr_table + i;
        if (!is_occupied(c_state))
            continue;

        state_print(c_state, indent + 1);
    }
}

int main()
{
    drop(state_free) state_t root;
    state_init(&root);

    for (size_t i = ____tok_kw_start + 1; i < ____tok_kw_end; ++i)
        state_insert(&root, as_slice(token_kind_str_repr((token_kind_t)i)), i);

    state_print(&root, 0);

    println("");
    char const *samples[] = {"in", "ino",  "inputu", "test", "il",
                             "if", "else", "true",   "True"};
    for (size_t i = 0; i < (sizeof(samples) / sizeof(samples[0])); ++i)
    {
        char const *sample = samples[i];
        uint8_t data = state_get(&root, as_slice(sample));

        if (data != 0)
            println("%s: %s", sample, token_kind_str_repr(data));
        else
            println("%s: identifier", sample);
    }

    return 0;
}

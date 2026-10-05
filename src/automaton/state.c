#include <ctype.h>
#include <stdlib.h>

#include "automaton/state.h"
#include "core/string/slice.h"
#include "lexical/token.h"
#include "utils/control.h"
#include "utils/guard.h"
#include "utils/log.h"

size_t const LOWER_ALPHA_OFS = 0, UPPER_ALPHA_OFS = ('z' - 'a') + 1,
             NUM_OFS = UPPER_ALPHA_OFS + (('Z' - 'A') + 1),
             SYM_OFS = NUM_OFS + (('9' - '0') + 1);

size_t const TR_TABLE_SIZE = SYM_OFS + 1 /* NOTE: Number of symbols
                                          */
    ;

size_t char_idx_of(char c)
{
    if (isalnum(c))
    {
        return isalpha(c) ? islower(c) ? LOWER_ALPHA_OFS + (c - 'a')
                                       : UPPER_ALPHA_OFS + (c - 'A')
                          : NUM_OFS + (c - '0');
    }

    if (c != '_')
        panic("invalid character");

    return SYM_OFS;
}

void state_init(state_t *state)
{
    *state = (state_t){0};
    state->tr_table = calloc(sizeof(state_t), TR_TABLE_SIZE);
}

void state_free(state_t *state)
{
    assert(is_init((*state)), "cannot free uninitialized state");

    if (state->tr_table == NULL)
        return;

    for (size_t i = 0; i < TR_TABLE_SIZE; ++i)
    {
        state_t *c_state = state->tr_table + i;
        if (is_occupied(*c_state))
            state_free(c_state);
    }

    free(state->tr_table);
}

void state_multi_insert(state_t *state, slice(char) char_seq, uint8_t acc_data)
{
    assert(is_init((*state)), "cannot insert on uninitialized states");

    state_t *curr = state;
    for (size_t i = 0; i < char_seq.n; ++i)
    {
        char c = char_seq.data[i];
        state_t *c_state = curr->tr_table + char_idx_of(c);

        if (is_occupied((*c_state)))
            state_multi_insert(
                c_state, as_slicen(char_seq.data + i + 1, char_seq.n - (i + 1)),
                acc_data);
        else
        {
            state_init(c_state);
            c_state->input = c;
        }

        curr = c_state;
    }

    curr->data = acc_data;
}

void state_insert(state_t *state, char input_sym, uint8_t data)
{
    state_multi_insert(state, as_slice((char const[]){input_sym}), data);
}

uint8_t state_get(state_t *root, slice(char) char_seq)
{
    assert(is_init((*root)), "cannot retrieve data of uninitialized states");

    state_t *curr = root;
    for (size_t i = 0; i < char_seq.n; ++i)
    {
        char c = char_seq.data[i];
        state_t *c_state = curr->tr_table + char_idx_of(c);

        if (!is_occupied((*c_state)))
            return c_state->data;

        curr = c_state;
    }

    return curr->data;
}

void state_print(state_t *state, uint32_t indent)
{
    assert(is_init((*state)), "cannot print uninitialized states");

    print("%*s(%c) ", (int)indent * 2, "", state->input);
    if (is_accepting((*state)))
        print("%s", token_kind_str_repr(state->data));

    for (size_t i = 0; i < TR_TABLE_SIZE; ++i)
    {
        state_t *c_state = state->tr_table + i;
        if (!is_occupied((*c_state)))
            continue;

        state_print(c_state, indent + 1);
    }
}

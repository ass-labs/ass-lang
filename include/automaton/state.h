#ifndef __AUTOMATON_STATE_H__
#define __AUTOMATON_STATE_H__

#include <stddef.h>
#include <stdint.h>

#include "core/collection/slice.h"
#include "core/string/slice.h"

typedef struct state_t
{
    struct state_t *tr_table;
    uint8_t data;
    char input;
} state_t;

#define is_init(state) ((state).tr_table != NULL)
#define is_accepting(state) ((state).data != 0)
#define is_occupied(state) ((state).input != '\0')

extern size_t const LOWER_ALPHA_OFS, UPPER_ALPHA_OFS, NUM_OFS, SYM_OFS;
extern size_t const TR_TABLE_SIZE;

size_t char_idx_of(char c);

void state_init(state_t *state);
void state_free(state_t *state);

void state_insert(state_t *state, char input_sym, uint8_t data);
void state_multi_insert(state_t *state, slice(char) char_seq, uint8_t acc_data);

uint8_t state_get(state_t *root, slice(char) char_seq);

void state_print(state_t *state, uint32_t indent);

#endif

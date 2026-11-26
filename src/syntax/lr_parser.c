#include <stdlib.h>
#include <string.h>

#include "core/utils/bi_defs.h"
#include "syntax/lr_parser.h"
#include "utils/guard.h"
#include "utils/log.h"

#define MAX_STATES 512
#define MAX_ITEMS 512

/// An LR(0) item `A -> α • β`, encoded as `prod << 4 | dot` so item sets can
/// be sorted and compared as plain integers.
typedef uint32_t item_t;

#define item(prod, dot) ((item_t)(prod) << 4 | (item_t)(dot))
#define item_prod(it) ((it) >> 4)
#define item_dot(it) ((it) & 0xF)

/// `ACTION` entries: 0 is an error, `n > 0` shifts and goes to state `n - 1`,
/// `n < 0` reduces by production `-n - 1`.
typedef int16_t action_t;

#define ACTION_ACCEPT INT16_MAX
#define shift_action(state) ((action_t)((state) + 1))
#define reduce_action(prod) ((action_t)(-(int)(prod) - 1))

static bool built;
static size_t n_states;
static item_t *kernels[MAX_STATES];
static size_t kernel_lens[MAX_STATES];

static action_t ACTION[MAX_STATES][N_TERMINALS];
/// Destination state of the transition on a nonterminal, -1 if none.
static int16_t GOTO[MAX_STATES][N_NONTERMINALS];

static bool nullable[N_NONTERMINALS];
static bool first[N_NONTERMINALS][N_TERMINALS];
static bool follow[N_NONTERMINALS][N_TERMINALS];

#define nt_idx(sym) ((size_t)((sym) - NT_BASE))

static bool set_union(bool *dst, bool const *src)
{
    bool changed = false;
    for (size_t t = 0; t < N_TERMINALS; ++t)
        if (src[t] && !dst[t])
            dst[t] = changed = true;

    return changed;
}

/// Adds FIRST(rhs[from..]) to `set`, returning whether that suffix is
/// nullable.
static bool add_first_of(bool *set, production_t const *prod, size_t from,
                         bool *changed)
{
    for (size_t i = from; i < prod->len; ++i)
    {
        gsym_t sym = prod->rhs[i];
        if (is_terminal(sym))
        {
            if (!set[sym])
                set[sym] = *changed = true;
            return false;
        }

        if (set_union(set, first[nt_idx(sym)]))
            *changed = true;
        if (!nullable[nt_idx(sym)])
            return false;
    }

    return true;
}

static void compute_first_follow(void)
{
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (size_t p = 0; p < N_PRODUCTIONS; ++p)
        {
            production_t const *prod = &PRODUCTIONS[p];
            size_t lhs = nt_idx(prod->lhs);

            if (add_first_of(first[lhs], prod, 0, &changed) && !nullable[lhs])
                nullable[lhs] = changed = true;
        }
    }

    follow[nt_idx(NT_START)][TOK_EOF] = true;

    changed = true;
    while (changed)
    {
        changed = false;
        for (size_t p = 0; p < N_PRODUCTIONS; ++p)
        {
            production_t const *prod = &PRODUCTIONS[p];
            for (size_t i = 0; i < prod->len; ++i)
            {
                gsym_t sym = prod->rhs[i];
                if (is_terminal(sym))
                    continue;

                bool *set = follow[nt_idx(sym)];
                if (add_first_of(set, prod, i + 1, &changed) &&
                    set_union(set, follow[nt_idx(prod->lhs)]))
                    changed = true;
            }
        }
    }
}

/// Expands the kernel `items[..n]` into its closure in place, returning the
/// new number of items.
static size_t closure(item_t *items, size_t n)
{
    bool added[N_NONTERMINALS] = {0};

    for (size_t i = 0; i < n; ++i)
    {
        production_t const *prod = &PRODUCTIONS[item_prod(items[i])];
        size_t dot = item_dot(items[i]);
        if (dot >= prod->len || is_terminal(prod->rhs[dot]))
            continue;

        gsym_t nt = prod->rhs[dot];
        if (added[nt_idx(nt)])
            continue;
        added[nt_idx(nt)] = true;

        for (size_t p = 0; p < N_PRODUCTIONS; ++p)
        {
            if (PRODUCTIONS[p].lhs != nt)
                continue;

            assert(n < MAX_ITEMS, "too many LR items in a state");
            items[n++] = item(p, 0);
        }
    }

    return n;
}

static int item_cmp(void const *a, void const *b)
{
    item_t x = *(item_t const *)a, y = *(item_t const *)b;
    return (x > y) - (x < y);
}

static size_t find_or_add_state(item_t *kernel, size_t n)
{
    qsort(kernel, n, sizeof(item_t), item_cmp);

    for (size_t s = 0; s < n_states; ++s)
        if (kernel_lens[s] == n &&
            memcmp(kernels[s], kernel, n * sizeof(item_t)) == 0)
            return s;

    assert(n_states < MAX_STATES, "too many LR states");

    item_t *copy = malloc(n * sizeof(item_t));
    if (copy == NULL)
        panic("failed to allocate LR state");
    memcpy(copy, kernel, n * sizeof(item_t));

    kernels[n_states] = copy, kernel_lens[n_states] = n;
    return n_states++;
}

static void describe_action(char *buf, size_t n, action_t action)
{
    if (action == ACTION_ACCEPT)
        snprintf(buf, n, "accept");
    else if (action > 0)
        snprintf(buf, n, "shift to state %d", action - 1);
    else
        snprintf(buf, n, "reduce by %s production #%d",
                 gsym_name(PRODUCTIONS[-action - 1].lhs), -action - 1);
}

static void set_action(size_t state, gsym_t term, action_t action)
{
    action_t prev = ACTION[state][term];
    if (prev != 0 && prev != action)
    {
        char a[96], b[96];
        describe_action(a, sizeof(a), prev);
        describe_action(b, sizeof(b), action);
        panic("SLR conflict in state %zu on `%s`: %s vs %s", state,
              token_kind_name((token_kind_t)term), a, b);
    }

    ACTION[state][term] = action;
}

static void lr_build(void)
{
    if (built)
        return;
    built = true;

    compute_first_follow();
    memset(GOTO, -1, sizeof(GOTO));

    item_t start = item(0, 0);
    find_or_add_state(&start, 1);

    static item_t items[MAX_ITEMS], next[MAX_ITEMS];
    for (size_t s = 0; s < n_states; ++s)
    {
        memcpy(items, kernels[s], kernel_lens[s] * sizeof(item_t));
        size_t n = closure(items, kernel_lens[s]);

        // Transitions: one per symbol appearing after a dot.
        for (gsym_t sym = 0; sym < ____nt_end; ++sym)
        {
            size_t next_n = 0;
            for (size_t i = 0; i < n; ++i)
            {
                production_t const *prod = &PRODUCTIONS[item_prod(items[i])];
                size_t dot = item_dot(items[i]);
                if (dot < prod->len && prod->rhs[dot] == sym)
                    next[next_n++] = items[i] + 1;
            }

            if (next_n == 0)
                continue;

            size_t dest = find_or_add_state(next, next_n);
            if (is_terminal(sym))
                set_action(s, sym, shift_action(dest));
            else
                GOTO[s][nt_idx(sym)] = (int16_t)dest;
        }

        // Reductions on the FOLLOW set of each completed item.
        for (size_t i = 0; i < n; ++i)
        {
            size_t p = item_prod(items[i]);
            if (item_dot(items[i]) != PRODUCTIONS[p].len)
                continue;

            if (p == 0)
            {
                set_action(s, TOK_EOF, ACTION_ACCEPT);
                continue;
            }

            bool const *set = follow[nt_idx(PRODUCTIONS[p].lhs)];
            for (gsym_t t = 0; t < N_TERMINALS; ++t)
                if (set[t])
                    set_action(s, t, reduce_action(p));
        }
    }
}

size_t lr_state_count(void)
{
    lr_build();
    return n_states;
}

/// Whether `term` would eventually be shifted (or accepted) from the state
/// stack `states`. SLR tables reduce on the whole FOLLOW set of a nonterminal,
/// so a terminal with an action in the top state may still fail after a few
/// reductions; this filters them out of the "expected" list of errors.
static bool can_shift(vec(size_t) const *states, gsym_t term)
{
    static size_t stack[4096];
    if (states->len > sizeof(stack) / sizeof(stack[0]))
        return ACTION[states->data[states->len - 1]][term] != 0;

    memcpy(stack, states->data, states->len * sizeof(size_t));
    size_t len = states->len;

    loop
    {
        action_t action = ACTION[stack[len - 1]][term];
        if (action == 0)
            return false;
        if (action > 0)
            return true;

        production_t const *prod = &PRODUCTIONS[-action - 1];
        len -= prod->len;
        stack[len] = (size_t)GOTO[stack[len - 1]][nt_idx(prod->lhs)];
        ++len;
    }
}

static void syntax_error(char const *file_path, vec(size_t) const *states,
                         token_t const *tok)
{
    char expected[1024] = "", desc[64];
    size_t len = 0, count = 0, total = 0;

    bool valid[N_TERMINALS];
    for (gsym_t t = 0; t < N_TERMINALS; ++t)
        total += valid[t] = can_shift(states, t);

    for (gsym_t t = 0; t < N_TERMINALS; ++t)
    {
        if (!valid[t])
            continue;

        terminal_desc(desc, sizeof(desc), t);
        char const *sep = count == 0           ? ""
                          : count + 1 == total ? " or "
                                               : ", ";
        len += snprintf(expected + len, sizeof(expected) - len, "%s%s", sep,
                        desc);
        ++count;

        if (len >= sizeof(expected))
            break;
    }

    if (tok->kind == TOK_EOF)
        eprintln("%s:%u:%u: syntax error (LR): expected %s, found end of file",
                 file_path, tok->row, tok->col, expected);
    else
        eprintln("%s:%u:%u: syntax error (LR): expected %s, found `" slice_fmt
                 "`",
                 file_path, tok->row, tok->col, expected, use(tok->lexeme));
}

bool lr_parse(parse_tree_t *tree, char const *file_path,
              token_t const *const *toks, size_t n)
{
    (void)n;
    lr_build();

    vec(size_t) states;
    vec(pt_node_ptr_t) nodes;
    vec_new(size_t)(&states, NULL, 64);
    vec_new(pt_node_ptr_t)(&nodes, NULL, 64);
    vec_push(size_t)(&states, 0);

    bool ok = false;
    size_t pos = 0;
    loop
    {
        size_t state = states.data[states.len - 1];
        token_t const *tok = toks[pos];
        action_t action = ACTION[state][tok->kind];

        if (action == ACTION_ACCEPT)
        {
            tree->root = nodes.data[nodes.len - 1];
            ok = true;
            break;
        }

        if (action > 0)
        {
            vec_push(pt_node_ptr_t)(&nodes, pt_new_leaf(tree, tok));
            vec_push(size_t)(&states, (size_t)(action - 1));
            ++pos;
            continue;
        }

        if (action == 0)
        {
            syntax_error(file_path, &states, tok);
            break;
        }

        // Reduce: pop the right-hand side, push the left-hand side.
        production_t const *prod = &PRODUCTIONS[-action - 1];
        pt_node_t *node = pt_new_node(tree, prod->lhs);

        size_t base = nodes.len - prod->len;
        for (size_t i = base; i < nodes.len; ++i)
            pt_add_child(node, nodes.data[i]);
        nodes.len = base;
        states.len -= prod->len;

        int16_t dest = GOTO[states.data[states.len - 1]][nt_idx(prod->lhs)];
        assert(dest >= 0, "missing LR goto on %s", gsym_name(prod->lhs));

        vec_push(pt_node_ptr_t)(&nodes, node);
        vec_push(size_t)(&states, (size_t)dest);
    }

    vec_free(size_t)(&states);
    vec_free(pt_node_ptr_t)(&nodes);
    return ok;
}

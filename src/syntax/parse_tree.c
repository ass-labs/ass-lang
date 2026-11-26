#include <string.h>

#include "syntax/parse_tree.h"

define_vec(pt_node_ptr_t);

static void pt_node_destr(pt_node_ptr_t *node)
{
    if (!is_terminal((*node)->sym))
        vec_free(pt_node_ptr_t)(&(*node)->children);
    free(*node);
}

void parse_tree_init(parse_tree_t *tree)
{
    vec_new(pt_node_ptr_t)(&tree->nodes, pt_node_destr, 64);
    tree->root = NULL;
}

void parse_tree_free(parse_tree_t *tree)
{
    vec_free(pt_node_ptr_t)(&tree->nodes);
}

static pt_node_t *pt_alloc(parse_tree_t *tree)
{
    pt_node_t *node = malloc(sizeof(pt_node_t));
    if (node == NULL)
        panic("failed to allocate parse tree node");

    vec_push(pt_node_ptr_t)(&tree->nodes, node);
    return node;
}

pt_node_t *pt_new_leaf(parse_tree_t *tree, token_t const *tok)
{
    pt_node_t *node = pt_alloc(tree);
    node->sym = (gsym_t)tok->kind, node->tok = tok;

    return node;
}

pt_node_t *pt_new_node(parse_tree_t *tree, gsym_t nt)
{
    pt_node_t *node = pt_alloc(tree);
    node->sym = nt, node->tok = NULL;
    vec_new(pt_node_ptr_t)(&node->children, NULL, 4);

    return node;
}

void pt_add_child(pt_node_t *parent, pt_node_t *child)
{
    if (!is_hidden(child->sym))
    {
        vec_push(pt_node_ptr_t)(&parent->children, child);
        return;
    }

    // The children of a hidden node were already spliced when they were added.
    for (size_t i = 0; i < child->children.len; ++i)
        vec_push(pt_node_ptr_t)(&parent->children, child->children.data[i]);
}

bool pt_equal(pt_node_t const *a, pt_node_t const *b)
{
    if (a->sym != b->sym)
        return false;

    if (is_terminal(a->sym))
        return a->tok == b->tok;

    if (a->children.len != b->children.len)
        return false;

    for (size_t i = 0; i < a->children.len; ++i)
        if (!pt_equal(a->children.data[i], b->children.data[i]))
            return false;

    return true;
}

static void write_sentential_form(FILE *out, vec(pt_node_ptr_t) const *form)
{
    if (form->len == 0)
        fputs("ε", out);

    for (size_t i = 0; i < form->len; ++i)
    {
        pt_node_t const *node = form->data[i];
        if (i > 0)
            fputc(' ', out);

        if (is_terminal(node->sym))
            fwrite(node->tok->lexeme.data, 1, node->tok->lexeme.n, out);
        else
            fputs(gsym_name(node->sym), out);
    }

    fputc('\n', out);
}

/// Index of the leftmost (or rightmost) nonterminal of `form`, or `form->len`
/// if it only has terminals left.
static size_t find_nonterminal(vec(pt_node_ptr_t) const *form, bool rightmost)
{
    for (size_t k = 0; k < form->len; ++k)
    {
        size_t i = rightmost ? form->len - 1 - k : k;
        if (!is_terminal(form->data[i]->sym))
            return i;
    }

    return form->len;
}

void pt_write_derivation(FILE *out, pt_node_t const *root, bool rightmost)
{
    vec(pt_node_ptr_t) form, next;
    vec_new(pt_node_ptr_t)(&form, NULL, 16);
    vec_new(pt_node_ptr_t)(&next, NULL, 16);

    vec_push(pt_node_ptr_t)(&form, (pt_node_t *)root);

    char const *root_name = gsym_name(root->sym);
    size_t indent = strlen(root_name);
    fputs(root_name, out);

    size_t idx;
    for (bool first = true;
         (idx = find_nonterminal(&form, rightmost)) < form.len; first = false)
    {
        // next = form[..idx] + children + form[idx + 1..]
        next.len = 0;
        pt_node_t const *expanded = form.data[idx];
        for (size_t i = 0; i < form.len; ++i)
        {
            if (i != idx)
            {
                vec_push(pt_node_ptr_t)(&next, form.data[i]);
                continue;
            }

            for (size_t j = 0; j < expanded->children.len; ++j)
                vec_push(pt_node_ptr_t)(&next, expanded->children.data[j]);
        }

        vec(pt_node_ptr_t) tmp = form;
        form = next, next = tmp;

        // Continuation lines align their `=>` under the first one.
        if (!first)
            fprintf(out, "%*s", (int)indent, "");
        fputs(" => ", out);
        write_sentential_form(out, &form);
    }

    vec_free(pt_node_ptr_t)(&form);
    vec_free(pt_node_ptr_t)(&next);
}

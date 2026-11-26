#ifndef __SYNTAX_PARSE_TREE_H__
#define __SYNTAX_PARSE_TREE_H__

#include <stdbool.h>
#include <stdio.h>

#include "core/collection/vec.h"
#include "lexical/token.h"
#include "syntax/grammar.h"

typedef struct pt_node_t pt_node_t;
typedef pt_node_t *pt_node_ptr_t;
declare_vec(pt_node_ptr_t);

/// A node of the concrete parse tree. Terminals are leaves holding their
/// token, nonterminals hold the symbols of the production they were derived
/// with (with hidden nonterminals spliced in).
struct pt_node_t
{
    gsym_t sym;
    token_t const *tok;
    vec(pt_node_ptr_t) children;
};

/// Owns every node of a parse tree, so a failed parse can be dropped without
/// walking a half-built tree.
typedef struct
{
    vec(pt_node_ptr_t) nodes;
    pt_node_t *root;
} parse_tree_t;

void parse_tree_init(parse_tree_t *tree);
void parse_tree_free(parse_tree_t *tree);

pt_node_t *pt_new_leaf(parse_tree_t *tree, token_t const *tok);
pt_node_t *pt_new_node(parse_tree_t *tree, gsym_t nt);

/// Appends `child` to `parent`, splicing the children of hidden nonterminals
/// in its place.
void pt_add_child(pt_node_t *parent, pt_node_t *child);

/// Whether both trees derive the same symbols and tokens.
bool pt_equal(pt_node_t const *a, pt_node_t const *b);

/// Writes the derivation of the tree, one sentential form per line (e.g.
/// `<program> => <stmt_list>`), expanding the leftmost or the rightmost
/// nonterminal at every step.
void pt_write_derivation(FILE *out, pt_node_t const *root, bool rightmost);

#endif

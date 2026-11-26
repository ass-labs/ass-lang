#include "syntax/analyzer.h"
#include "config.h"
#include "syntax/ll_parser.h"
#include "syntax/lower.h"
#include "syntax/lr_parser.h"

define_vec(token_ref_t);

void parser_new(parser_t *parser, lexer_t const *lexer)
{
    parser->file_path = lexer->file_path;
    parser->eof = (token_t){.lexeme = as_slicen(lexer->src.data + lexer->src.n, 0),
                            .kind = TOK_EOF,
                            .row = (uint32_t)lexer->row,
                            .col = (uint32_t)lexer->col};

    vec_new(token_ref_t)(&parser->toks, NULL, lexer->tokens.len + 1);
    for (size_t i = 0; i < lexer->tokens.len; ++i)
    {
        token_t const *tok = &lexer->tokens.data[i];
        if (tok->kind != TOK_LINE_COMMENT && tok->kind != TOK_BLOCK_COMMENT)
            vec_push(token_ref_t)(&parser->toks, tok);
    }
    vec_push(token_ref_t)(&parser->toks, &parser->eof);

    parse_tree_init(&parser->ll_tree);
    parse_tree_init(&parser->lr_tree);
}

void parser_free(parser_t *parser)
{
    vec_free(token_ref_t)(&parser->toks);
    parse_tree_free(&parser->ll_tree);
    parse_tree_free(&parser->lr_tree);
}

bool parser_parse(parser_t *parser, program_node_t *program)
{
    bool ok = true;

    if (CONFIG.p_conf & PCONF_LL)
        ok &= ll_parse(&parser->ll_tree, parser->file_path,
                       parser->toks.data, parser->toks.len);

    if (CONFIG.p_conf & PCONF_LR)
        ok &= lr_parse(&parser->lr_tree, parser->file_path,
                       parser->toks.data, parser->toks.len);

    pt_node_t const *ll = parser->ll_tree.root, *lr = parser->lr_tree.root;
    if (!ok)
        return false;

    // Both parsers implement the same grammar, so they must agree.
    if (ll != NULL && lr != NULL && !pt_equal(ll, lr))
        panic("the LL and LR parsers built different parse trees for '%s'",
              parser->file_path);

    pt_node_t const *root = ll != NULL ? ll : lr;
    if (root != NULL)
        lower_program(root, program);

    return true;
}

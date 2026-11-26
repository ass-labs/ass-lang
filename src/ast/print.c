#include "ast/print.h"
#include "ast/block.h"

static void write_token(FILE *out, token_t const *tok)
{
    fwrite(tok->lexeme.data, 1, tok->lexeme.n, out);
}

static void write_expr(FILE *out, expr_t const *expr)
{
    switch (expr->kind)
    {
        case EXPR_LITERAL:
            write_token(out, &expr->data.literal);
            break;
        case EXPR_IDENT:
            write_token(out, &expr->data.ident);
            break;
        case EXPR_UNARY:
            fputc('(', out);
            write_token(out, &expr->data.unary.op);
            fputc(' ', out);
            write_expr(out, expr->data.unary.operand);
            fputc(')', out);
            break;
        case EXPR_BINARY:
            fputc('(', out);
            write_token(out, &expr->data.binary.op);
            fputc(' ', out);
            write_expr(out, expr->data.binary.lhs);
            fputc(' ', out);
            write_expr(out, expr->data.binary.rhs);
            fputc(')', out);
            break;
    }
}

static void indent(FILE *out, size_t depth)
{
    fprintf(out, "\n%*s", (int)(depth * 2), "");
}

static void write_stmt(FILE *out, stmt_t const *stmt, size_t depth);

static void write_block(FILE *out, block_t const *block, size_t depth)
{
    fputs("(block", out);
    for (size_t i = 0; i < block->stmts.len; ++i)
    {
        indent(out, depth + 1);
        write_stmt(out, &block->stmts.data[i], depth + 1);
    }
    fputc(')', out);
}

static void write_stmt(FILE *out, stmt_t const *stmt, size_t depth)
{
    switch (stmt->kind)
    {
        case STMT_DECL:
        {
            struct decl_stmt_t const *decl = &stmt->data.decl;
            fputs("(decl ", out);
            write_token(out, &decl->ty.data.nominal);

            for (size_t i = 0; i < decl->items.len; ++i)
            {
                decl_item_t const *item = &decl->items.data[i];
                indent(out, depth + 1);

                if (item->val == NULL)
                {
                    write_token(out, &item->name);
                    continue;
                }

                fputc('(', out);
                write_token(out, &item->name);
                fputc(' ', out);
                write_expr(out, item->val);
                fputc(')', out);
            }
            fputc(')', out);
            break;
        }

        case STMT_ASSIGN:
            fputs("(assign ", out);
            write_token(out, &stmt->data.assign.target);
            fputc(' ', out);
            write_token(out, &stmt->data.assign.op);
            fputc(' ', out);
            write_expr(out, stmt->data.assign.val);
            fputc(')', out);
            break;

        case STMT_IF:
            fputs("(if ", out);
            write_expr(out, stmt->data._if.cond);
            indent(out, depth + 1);
            write_block(out, stmt->data._if.consequent, depth + 1);
            if (stmt->data._if.alternate != NULL)
            {
                indent(out, depth + 1);
                write_block(out, stmt->data._if.alternate, depth + 1);
            }
            fputc(')', out);
            break;

        case STMT_WHILE:
            fputs("(while ", out);
            write_expr(out, stmt->data._while.cond);
            indent(out, depth + 1);
            write_block(out, stmt->data._while.body, depth + 1);
            fputc(')', out);
            break;

        case STMT_INPUT:
            fputs("(read", out);
            for (size_t i = 0; i < stmt->data.input.targets.len; ++i)
            {
                fputc(' ', out);
                write_token(out, &stmt->data.input.targets.data[i]);
            }
            fputc(')', out);
            break;

        case STMT_OUTPUT:
            fputs("(write", out);
            for (size_t i = 0; i < stmt->data.output.vals.len; ++i)
            {
                fputc(' ', out);
                write_expr(out, stmt->data.output.vals.data[i]);
            }
            fputc(')', out);
            break;
    }
}

void ast_write(FILE *out, program_node_t const *program)
{
    fputs("(program", out);
    for (size_t i = 0; i < program->stmts.len; ++i)
    {
        indent(out, 1);
        write_stmt(out, &program->stmts.data[i], 1);
    }
    fputs(")\n", out);
}

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "ast/block.h"
#include "semantic/analyzer.h"
#include "utils/control.h"

define_vec(symbol_t);

// ---------------------------------------------------------------------------
// Reporting
// ---------------------------------------------------------------------------

/// Reports a diagnostic at `at`. Returns `NULL` (nothing reported) while the
/// analyzer is silent.
__attribute__((format(printf, 4, 5))) static diag_t *
report(sema_t *sema, diag_severity_t severity, token_t const *at,
       char const *fmt, ...)
{
    if (sema->silent > 0)
        return NULL;

    char msg[DIAG_MSG_MAX];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    return diag_report(&sema->diags, severity, at->row, at->col, "%s", msg);
}

/// Attaches a note at the declaration of `sym` to `diag`. `fmt` formats the
/// variable's name.
static void note_decl(diag_t *diag, symbol_t const *sym, char const *fmt)
{
    if (diag != NULL)
        diag_note(diag, sym->name.row, sym->name.col, fmt,
                  use(sym->name.lexeme));
}

// ---------------------------------------------------------------------------
// Scopes and symbols
// ---------------------------------------------------------------------------

static bool same_name(string_slice_t a, string_slice_t b)
{
    return a.n == b.n && memcmp(a.data, b.data, a.n) == 0;
}

/// Innermost visible variable named `name`, or `NULL`.
static symbol_t *lookup(sema_t *sema, string_slice_t name)
{
    for (size_t i = sema->syms.len; i-- > 0;)
        if (same_name(sema->syms.data[i].name.lexeme, name))
            return &sema->syms.data[i];
    return NULL;
}

/// Variable named `name` declared in the current scope, or `NULL`.
static symbol_t *lookup_current_scope(sema_t *sema, string_slice_t name)
{
    for (size_t i = sema->syms.len; i-- > 0;)
    {
        symbol_t *sym = &sema->syms.data[i];
        if (sym->depth != sema->depth)
            break;
        if (same_name(sym->name.lexeme, name))
            return sym;
    }
    return NULL;
}

static void warn_if_unused(sema_t *sema, symbol_t const *sym)
{
    // `_x` marks a variable as intentionally unused.
    if (sym->read || (sym->name.lexeme.n > 0 && sym->name.lexeme.data[0] == '_'))
        return;

    if (sym->assigned)
        report(sema, DIAG_WARNING, &sym->name,
               "variable `" slice_fmt "` is assigned but its value is never read",
               use(sym->name.lexeme));
    else
        report(sema, DIAG_WARNING, &sym->name, "unused variable `" slice_fmt "`",
               use(sym->name.lexeme));
}

static void scope_enter(sema_t *sema) { ++sema->depth; }

/// Drops the variables of the current scope, warning about the unused ones.
static void scope_exit(sema_t *sema)
{
    while (sema->syms.len > 0 &&
           sema->syms.data[sema->syms.len - 1].depth == sema->depth)
    {
        warn_if_unused(sema, &sema->syms.data[sema->syms.len - 1]);
        --sema->syms.len;
    }

    if (sema->depth > 0)
        --sema->depth;
}

// ---------------------------------------------------------------------------
// Initialization state, saved and merged around branches and loops
// ---------------------------------------------------------------------------

typedef struct
{
    /// One entry per variable alive when the state was saved.
    bool *def, *maybe;
    size_t len;
} init_state_t;

static void init_state_save(sema_t const *sema, init_state_t *state)
{
    state->len = sema->syms.len;
    state->def = malloc(state->len * sizeof(bool) + 1);
    state->maybe = malloc(state->len * sizeof(bool) + 1);
    if (state->def == NULL || state->maybe == NULL)
        panic("failed to allocate initialization state");

    for (size_t i = 0; i < state->len; ++i)
    {
        state->def[i] = sema->syms.data[i].def_init;
        state->maybe[i] = sema->syms.data[i].maybe_init;
    }
}

static void init_state_restore(sema_t *sema, init_state_t const *state)
{
    for (size_t i = 0; i < state->len; ++i)
    {
        sema->syms.data[i].def_init = state->def[i];
        sema->syms.data[i].maybe_init = state->maybe[i];
    }
}

static void init_state_free(init_state_t *state)
{
    free(state->def);
    free(state->maybe);
}

// ---------------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------------

/// Token where `expr` starts in the source.
static token_t const *expr_start(expr_t const *expr)
{
    switch (expr->kind)
    {
        case EXPR_LITERAL:
            return &expr->data.literal;
        case EXPR_IDENT:
            return &expr->data.ident;
        case EXPR_UNARY:
            return &expr->data.unary.op;
        case EXPR_BINARY:
            return expr_start(expr->data.binary.lhs);
    }
    return NULL;
}

/// Checks that `sym` holds a value before it is read at `at`.
static void check_initialized(sema_t *sema, symbol_t *sym, token_t const *at)
{
    if (sym->def_init || sym->uninit_reported)
        return;

    diag_t *diag;
    if (sym->maybe_init)
        diag = report(sema, DIAG_WARNING, at,
                      "`" slice_fmt "` may be used before being initialized "
                      "(it is not assigned on every path reaching here)",
                      use(at->lexeme));
    else
        diag = report(sema, DIAG_ERROR, at,
                      "`" slice_fmt "` is used before being initialized",
                      use(at->lexeme));

    note_decl(diag, sym, "`" slice_fmt "` is declared here without a value");

    // Report each variable once, but only once it was actually reported
    // (not during the silent pass over a loop body).
    if (diag != NULL)
        sym->uninit_reported = true;
}

/// Looks up the variable `name` refers to, reporting it if it isn't declared.
static symbol_t *resolve(sema_t *sema, token_t const *name)
{
    symbol_t *sym = lookup(sema, name->lexeme);
    if (sym == NULL)
        report(sema, DIAG_ERROR, name,
               "cannot find variable `" slice_fmt "` in this scope",
               use(name->lexeme));
    return sym;
}

static bool is_arith_op(token_kind_t op)
{
    switch (op)
    {
        case TOK_PLUS:
        case TOK_MINUS:
        case TOK_STAR:
        case TOK_SLASH:
        case TOK_SLASH_SLASH:
        case TOK_PERCENT:
        case TOK_CARET:
            return true;
        default:
            return false;
    }
}

/// Type of `lhs <op> rhs` for an arithmetic operator on numeric operands.
/// `/` is true division, so it always gives a `dnum`; `//` is floor division.
static sem_ty_t arith_result(token_kind_t op, sem_ty_t lhs, sem_ty_t rhs)
{
    if (op == TOK_SLASH)
        return STY_DNUM;
    return lhs == STY_DNUM || rhs == STY_DNUM ? STY_DNUM : STY_WNUM;
}

static sem_ty_t check_expr(sema_t *sema, expr_t const *expr);

static sem_ty_t check_unary(sema_t *sema, struct unary_expr_t const *unary)
{
    sem_ty_t operand = check_expr(sema, unary->operand);
    token_t const *op = &unary->op;

    if (op->kind == TOK_KW_NOT)
    {
        if (operand != STY_BOOL && operand != STY_ERROR)
            report(sema, DIAG_ERROR, op,
                   "`NOT` expects a `bool` operand, found `%s`",
                   sem_ty_name(operand));
        return STY_BOOL;
    }

    // Unary `+` and `-`
    if (operand == STY_ERROR)
        return STY_ERROR;
    if (!sem_ty_is_numeric(operand))
    {
        report(sema, DIAG_ERROR, op,
               "unary `" slice_fmt "` cannot be applied to a `%s` value",
               use(op->lexeme), sem_ty_name(operand));
        return STY_ERROR;
    }
    return operand;
}

static sem_ty_t check_binary(sema_t *sema, struct binary_expr_t const *binary)
{
    sem_ty_t lhs = check_expr(sema, binary->lhs),
             rhs = check_expr(sema, binary->rhs);
    token_t const *op = &binary->op;
    bool has_error = lhs == STY_ERROR || rhs == STY_ERROR;

    if (is_arith_op(op->kind))
    {
        if (has_error)
            return STY_ERROR;
        if (!sem_ty_is_numeric(lhs) || !sem_ty_is_numeric(rhs))
        {
            report(sema, DIAG_ERROR, op,
                   "`" slice_fmt "` cannot be applied to `%s` and `%s` (it "
                   "needs `wnum` or `dnum` operands)",
                   use(op->lexeme), sem_ty_name(lhs), sem_ty_name(rhs));
            return STY_ERROR;
        }
        return arith_result(op->kind, lhs, rhs);
    }

    // The result of a comparison or logical operator is a `bool` even when
    // its operands are wrong, so the error doesn't spread.
    if (has_error)
        return STY_BOOL;

    switch (op->kind)
    {
        case TOK_KW_AND:
        case TOK_KW_OR:
            if (lhs != STY_BOOL || rhs != STY_BOOL)
                report(sema, DIAG_ERROR, op,
                       "`" slice_fmt "` expects `bool` operands, found `%s` "
                       "and `%s`",
                       use(op->lexeme), sem_ty_name(lhs), sem_ty_name(rhs));
            break;

        case TOK_EQ_EQ:
        case TOK_BANG_EQ:
            if (lhs == STY_STR || rhs == STY_STR)
                report(sema, DIAG_ERROR, op,
                       "string literals cannot be compared, they can only be "
                       "written");
            else if (lhs != rhs &&
                     !(sem_ty_is_numeric(lhs) && sem_ty_is_numeric(rhs)))
                report(sema, DIAG_ERROR, op,
                       "cannot compare a `%s` with a `%s` using `" slice_fmt
                       "`",
                       sem_ty_name(lhs), sem_ty_name(rhs), use(op->lexeme));
            break;

        case TOK_LT:
        case TOK_GT:
        case TOK_LT_EQ:
        case TOK_GT_EQ:
            if (!(sem_ty_is_numeric(lhs) && sem_ty_is_numeric(rhs)) &&
                !(lhs == STY_CHAR && rhs == STY_CHAR))
                report(sema, DIAG_ERROR, op,
                       "cannot compare a `%s` with a `%s` using `" slice_fmt
                       "` (it needs two numbers or two `char`s)",
                       sem_ty_name(lhs), sem_ty_name(rhs), use(op->lexeme));
            break;

        default:
            panic("unexpected binary operator %s", token_kind_name(op->kind));
    }

    return STY_BOOL;
}

static sem_ty_t check_expr(sema_t *sema, expr_t const *expr)
{
    switch (expr->kind)
    {
        case EXPR_LITERAL:
            return sem_ty_of_literal(expr->data.literal.kind);

        case EXPR_IDENT:
        {
            token_t const *name = &expr->data.ident;
            symbol_t *sym = resolve(sema, name);
            if (sym == NULL)
                return STY_ERROR;

            sym->read = true;
            check_initialized(sema, sym, name);
            return sym->ty;
        }

        case EXPR_UNARY:
            return check_unary(sema, &expr->data.unary);

        case EXPR_BINARY:
            return check_binary(sema, &expr->data.binary);
    }
    return STY_ERROR;
}

/// Checks the condition of an `if` or `while`, which must be a `bool`.
static void check_cond(sema_t *sema, expr_t const *cond, char const *stmt)
{
    sem_ty_t ty = check_expr(sema, cond);
    if (ty != STY_BOOL && ty != STY_ERROR)
        report(sema, DIAG_ERROR, expr_start(cond),
               "the condition of `%s` must be a `bool`, found `%s`", stmt,
               sem_ty_name(ty));
}

/// Whether `expr` divides with `/` anywhere, which makes it a `dnum`.
static bool has_true_division(expr_t const *expr)
{
    switch (expr->kind)
    {
        case EXPR_UNARY:
            return has_true_division(expr->data.unary.operand);
        case EXPR_BINARY:
            return expr->data.binary.op.kind == TOK_SLASH ||
                   has_true_division(expr->data.binary.lhs) ||
                   has_true_division(expr->data.binary.rhs);
        default:
            return false;
    }
}

/// Reports storing a `src` value in `name`, a variable of type `dst`, if the
/// types don't match. `at` is where the error is reported, and `hint` (or
/// `NULL`) is appended to a `dnum` to `wnum` error.
static void check_store(sema_t *sema, token_t const *at, string_slice_t name,
                        sem_ty_t dst, sem_ty_t src, char const *hint)
{
    if (src == STY_ERROR || sem_ty_assignable(dst, src))
        return;

    if (src == STY_STR)
        report(sema, DIAG_ERROR, at,
               "a string literal cannot be stored in `" slice_fmt
               "`, strings can only be written",
               use(name));
    else if (dst == STY_WNUM && src == STY_DNUM)
        report(sema, DIAG_ERROR, at,
               "a `dnum` value cannot be stored in `" slice_fmt
               "` of type `wnum`, its fractional part would be lost%s",
               use(name), hint != NULL ? hint : "");
    else
        report(sema, DIAG_ERROR, at,
               "mismatched types: `" slice_fmt "` is a `%s`, but the value "
               "is a `%s`",
               use(name), sem_ty_name(dst), sem_ty_name(src));
}

static char const *division_hint(expr_t const *val)
{
    return has_true_division(val) ? " (`/` always gives a `dnum`, use `//` "
                                    "for whole-number division)"
                                  : NULL;
}

// ---------------------------------------------------------------------------
// Statements
// ---------------------------------------------------------------------------

static void check_stmt(sema_t *sema, stmt_t const *stmt);

static void check_block(sema_t *sema, block_t const *block)
{
    scope_enter(sema);
    for (size_t i = 0; i < block->stmts.len; ++i)
        check_stmt(sema, &block->stmts.data[i]);
    scope_exit(sema);
}

static void check_decl(sema_t *sema, struct decl_stmt_t const *decl)
{
    sem_ty_t ty = sem_ty_from_keyword(decl->ty.data.nominal.kind);

    for (size_t i = 0; i < decl->items.len; ++i)
    {
        decl_item_t const *item = &decl->items.data[i];

        // The initializer is checked first: in `wnum x = x + 1` the `x` on
        // the right is not the one being declared.
        if (item->val != NULL)
            check_store(sema, &item->name, item->name.lexeme, ty,
                        check_expr(sema, item->val), division_hint(item->val));

        symbol_t *prev = lookup_current_scope(sema, item->name.lexeme);
        if (prev != NULL)
        {
            diag_t *diag = report(
                sema, DIAG_ERROR, &item->name,
                "`" slice_fmt "` is already declared in this scope",
                use(item->name.lexeme));
            note_decl(diag, prev, "previous declaration of `" slice_fmt "` here");
            continue;
        }

        symbol_t *outer = lookup(sema, item->name.lexeme);
        if (outer != NULL)
        {
            diag_t *diag =
                report(sema, DIAG_WARNING, &item->name,
                       "`" slice_fmt "` shadows a variable of an enclosing "
                       "scope",
                       use(item->name.lexeme));
            note_decl(diag, outer, "shadowed declaration of `" slice_fmt "` here");
        }

        bool init = item->val != NULL;
        *vec_expand(symbol_t)(&sema->syms) = (symbol_t){
            .name = item->name,
            .ty = ty,
            .depth = sema->depth,
            .def_init = init,
            .maybe_init = init,
        };
    }
}

/// Arithmetic operator a compound assignment applies, e.g. `+` for `+=`.
static token_kind_t compound_op(token_kind_t op)
{
    switch (op)
    {
        case TOK_PLUS_EQ:
            return TOK_PLUS;
        case TOK_MINUS_EQ:
            return TOK_MINUS;
        case TOK_STAR_EQ:
            return TOK_STAR;
        case TOK_SLASH_EQ:
            return TOK_SLASH;
        case TOK_SLASH_SLASH_EQ:
            return TOK_SLASH_SLASH;
        default:
            panic("unexpected assignment operator %s", token_kind_name(op));
    }
}

static void check_assign(sema_t *sema, struct assign_stmt_t const *assign)
{
    token_t const *target = &assign->target, *op = &assign->op;
    symbol_t *sym = resolve(sema, target);
    // The value is evaluated before it is stored.
    sem_ty_t val = check_expr(sema, assign->val);

    if (sym == NULL)
        return;

    // `vec_expand` in `check_expr` never runs (expressions declare nothing),
    // so `sym` is still valid here.
    if (op->kind == TOK_EQ)
        check_store(sema, op, target->lexeme, sym->ty, val,
                    division_hint(assign->val));
    else
    {
        // `x += v` reads `x`, so `x` must hold a value. It doesn't count as
        // using `x`: its new value still has to be read somewhere.
        check_initialized(sema, sym, target);

        if (val != STY_ERROR)
        {
            if (!sem_ty_is_numeric(sym->ty) || !sem_ty_is_numeric(val))
                report(sema, DIAG_ERROR, op,
                       "`" slice_fmt "` cannot be applied to `%s` and `%s` (it "
                       "needs `wnum` or `dnum` operands)",
                       use(op->lexeme), sem_ty_name(sym->ty),
                       sem_ty_name(val));
            else
                check_store(sema, op, target->lexeme, sym->ty,
                            arith_result(compound_op(op->kind), sym->ty, val),
                            op->kind == TOK_SLASH_EQ
                                ? " (`/=` always gives a `dnum`, use `//=` for "
                                  "whole-number division)"
                                : division_hint(assign->val));
        }
    }

    sym->def_init = sym->maybe_init = sym->assigned = true;
}

static void check_if(sema_t *sema, struct if_stmt_t const *_if)
{
    check_cond(sema, _if->cond, "if");

    init_state_t before, after_then;
    init_state_save(sema, &before);

    check_block(sema, _if->consequent);
    init_state_save(sema, &after_then);

    init_state_restore(sema, &before);
    if (_if->alternate != NULL)
        check_block(sema, _if->alternate);

    // A variable is definitely initialized after the `if` when both branches
    // initialize it (a missing `else` initializes nothing).
    for (size_t i = 0; i < before.len; ++i)
    {
        symbol_t *sym = &sema->syms.data[i];
        sym->def_init = sym->def_init && after_then.def[i];
        sym->maybe_init = sym->maybe_init || after_then.maybe[i];
    }

    init_state_free(&before);
    init_state_free(&after_then);
}

static void check_while(sema_t *sema, struct while_stmt_t const *_while)
{
    init_state_t before;
    init_state_save(sema, &before);

    // The body may run after itself, so what it assigns may already hold a
    // value at the start of the next iteration. A silent first pass finds
    // those variables.
    ++sema->silent;
    check_expr(sema, _while->cond);
    check_block(sema, _while->body);
    --sema->silent;

    // State at the head of the loop: only what was initialized before it is
    // definite, anything the body assigns is possible.
    for (size_t i = 0; i < before.len; ++i)
        sema->syms.data[i].def_init = before.def[i];

    check_cond(sema, _while->cond, "while");
    check_block(sema, _while->body);

    // The body may not run at all.
    for (size_t i = 0; i < before.len; ++i)
    {
        symbol_t *sym = &sema->syms.data[i];
        sym->def_init = before.def[i];
        sym->maybe_init = sym->maybe_init || before.maybe[i];
    }

    init_state_free(&before);
}

static void check_input(sema_t *sema, struct input_stmt_t const *input)
{
    for (size_t i = 0; i < input->targets.len; ++i)
    {
        symbol_t *sym = resolve(sema, &input->targets.data[i]);
        if (sym != NULL)
            sym->def_init = sym->maybe_init = sym->assigned = true;
    }
}

static void check_stmt(sema_t *sema, stmt_t const *stmt)
{
    switch (stmt->kind)
    {
        case STMT_DECL:
            check_decl(sema, &stmt->data.decl);
            break;
        case STMT_ASSIGN:
            check_assign(sema, &stmt->data.assign);
            break;
        case STMT_IF:
            check_if(sema, &stmt->data._if);
            break;
        case STMT_WHILE:
            check_while(sema, &stmt->data._while);
            break;
        case STMT_INPUT:
            check_input(sema, &stmt->data.input);
            break;
        case STMT_OUTPUT:
            // Any value can be written, string literals included.
            for (size_t i = 0; i < stmt->data.output.vals.len; ++i)
                check_expr(sema, stmt->data.output.vals.data[i]);
            break;
    }
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void sema_new(sema_t *sema, char const *file_path)
{
    vec_new(symbol_t)(&sema->syms, NULL, 16);
    diag_list_new(&sema->diags, file_path);
    sema->depth = 0;
    sema->silent = 0;
}

void sema_free(sema_t *sema)
{
    vec_free(symbol_t)(&sema->syms);
    diag_list_free(&sema->diags);
}

bool sema_analyze(sema_t *sema, program_node_t const *program)
{
    for (size_t i = 0; i < program->stmts.len; ++i)
        check_stmt(sema, &program->stmts.data[i]);

    // The program's own scope ends with the file.
    scope_exit(sema);

    diag_list_print(&sema->diags, stderr);
    return sema->diags.err_count == 0;
}

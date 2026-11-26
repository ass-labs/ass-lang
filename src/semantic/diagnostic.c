#include <stdarg.h>
#include <stdlib.h>

#include "semantic/diagnostic.h"

define_vec(diag_t);

#define RED "\033[1;31m"
#define YELLOW "\033[1;33m"
#define CYAN "\033[1;36m"
#define RESET "\033[0m"

void diag_list_new(diag_list_t *list, char const *file_path)
{
    list->file_path = file_path;
    list->err_count = list->warn_count = 0;
    vec_new(diag_t)(&list->diags, NULL, 8);
}

void diag_list_free(diag_list_t *list) { vec_free(diag_t)(&list->diags); }

diag_t *diag_report(diag_list_t *list, diag_severity_t severity, uint32_t row,
                    uint32_t col, char const *fmt, ...)
{
    size_t seq = list->diags.len;
    diag_t *diag = vec_expand(diag_t)(&list->diags);
    *diag = (diag_t){
        .row = row, .col = col, .seq = seq, .severity = severity};

    va_list args;
    va_start(args, fmt);
    vsnprintf(diag->msg, sizeof(diag->msg), fmt, args);
    va_end(args);

    if (severity == DIAG_ERROR)
        ++list->err_count;
    else
        ++list->warn_count;

    return diag;
}

void diag_note(diag_t *diag, uint32_t row, uint32_t col, char const *fmt, ...)
{
    diag->has_note = true;
    diag->note_row = row;
    diag->note_col = col;

    va_list args;
    va_start(args, fmt);
    vsnprintf(diag->note, sizeof(diag->note), fmt, args);
    va_end(args);
}

static int diag_cmp(void const *a, void const *b)
{
    diag_t const *x = a, *y = b;
    if (x->row != y->row)
        return x->row < y->row ? -1 : 1;
    if (x->col != y->col)
        return x->col < y->col ? -1 : 1;
    return x->seq < y->seq ? -1 : x->seq > y->seq;
}

void diag_list_print(diag_list_t *list, FILE *out)
{
    qsort(list->diags.data, list->diags.len, sizeof(diag_t), diag_cmp);

    for (size_t i = 0; i < list->diags.len; ++i)
    {
        diag_t const *d = &list->diags.data[i];
        fprintf(out, "%s:%u:%u: %s: %s\n", list->file_path, d->row, d->col,
                d->severity == DIAG_ERROR ? RED "error" RESET
                                          : YELLOW "warning" RESET,
                d->msg);

        if (d->has_note)
            fprintf(out, "%s:%u:%u: " CYAN "note" RESET ": %s\n",
                    list->file_path, d->note_row, d->note_col, d->note);
    }
}

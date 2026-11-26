#ifndef __SEMANTIC_DIAGNOSTIC_H__
#define __SEMANTIC_DIAGNOSTIC_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "core/collection/vec.h"

typedef enum : uint8_t
{
    DIAG_ERROR,
    DIAG_WARNING,
} diag_severity_t;

#define DIAG_MSG_MAX 256

/// One error or warning, with an optional note pointing at a related place
/// (e.g. the previous declaration of a redefined variable).
typedef struct
{
    char msg[DIAG_MSG_MAX];
    char note[DIAG_MSG_MAX];
    uint32_t row, col, note_row, note_col;
    /// Order of reporting, keeps diagnostics at the same position in order.
    size_t seq;
    diag_severity_t severity;
    bool has_note;
} diag_t;

declare_vec(diag_t);

/// Collects diagnostics so they can be printed sorted by position: some are
/// only found late (e.g. unused variables, at the end of their scope).
typedef struct
{
    char const *file_path;
    vec(diag_t) diags;
    size_t err_count, warn_count;
} diag_list_t;

void diag_list_new(diag_list_t *list, char const *file_path);
void diag_list_free(diag_list_t *list);

/// Adds a diagnostic and returns it, so a note can be attached with
/// `diag_note`.
__attribute__((format(printf, 5, 6))) diag_t *
diag_report(diag_list_t *list, diag_severity_t severity, uint32_t row,
            uint32_t col, char const *fmt, ...);

__attribute__((format(printf, 4, 5))) void
diag_note(diag_t *diag, uint32_t row, uint32_t col, char const *fmt, ...);

/// Prints the diagnostics sorted by position.
void diag_list_print(diag_list_t *list, FILE *out);

#endif

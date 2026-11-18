#include <stdlib.h>

#include "ast/ty.h"
#include "utils/control.h"

void ty_init(ty_t *ty, ty_kind_t kind)
{
    ty->kind = kind;

    union ty_data_t *data = malloc(sizeof(union ty_data_t));
    if (data == NULL)
        panic("failed to allocate ty data buffer");

    ty->data = data;
}

void ty_free(ty_t *ty)
{
    // TODO: per kind clean-up
    free(ty->data);
}

#ifndef __AST_TY_H__
#define __AST_TY_H__

#include <stdint.h>

typedef struct
{
    union ty_data_t
    {
        struct nominal_ty_t
        {

        } nominal;
    } *data;

    enum ty_kind_t : uint8_t
    {
        TY_NOMINAL,
    } kind;
} ty_t;

typedef enum ty_kind_t ty_kind_t;
typedef struct nominal_ty_t ty_nominal_t;

void ty_init(ty_t *ty, ty_kind_t kind);
void ty_free(ty_t *ty);

#endif

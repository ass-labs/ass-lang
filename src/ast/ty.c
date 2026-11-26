#include "ast/ty.h"

ty_t ty_new_nominal(token_t name)
{
    return (ty_t){.data.nominal = name, .kind = TY_NOMINAL};
}

#include "semantic/types.h"
#include "utils/control.h"

char const *sem_ty_name(sem_ty_t ty)
{
    switch (ty)
    {
        case STY_ERROR:
            return "{error}";
        case STY_WNUM:
            return "wnum";
        case STY_DNUM:
            return "dnum";
        case STY_BOOL:
            return "bool";
        case STY_CHAR:
            return "char";
        case STY_STR:
            return "string";
    }
    return "?";
}

sem_ty_t sem_ty_from_keyword(token_kind_t kind)
{
    switch (kind)
    {
        case TOK_RW_WNUM:
            return STY_WNUM;
        case TOK_RW_DNUM:
            return STY_DNUM;
        case TOK_KW_BOOL:
            return STY_BOOL;
        case TOK_KW_CHAR:
            return STY_CHAR;
        default:
            panic("token kind %s is not a type", token_kind_name(kind));
    }
}

sem_ty_t sem_ty_of_literal(token_kind_t kind)
{
    switch (kind)
    {
        case TOK_INT:
            return STY_WNUM;
        case TOK_FLOAT:
            return STY_DNUM;
        case TOK_BOOL:
        case TOK_KW_TRUE:
        case TOK_KW_FALSE:
            return STY_BOOL;
        case TOK_CHAR:
            return STY_CHAR;
        case TOK_STR:
            return STY_STR;
        default:
            panic("token kind %s is not a literal", token_kind_name(kind));
    }
}

bool sem_ty_is_numeric(sem_ty_t ty) { return ty == STY_WNUM || ty == STY_DNUM; }

bool sem_ty_assignable(sem_ty_t dst, sem_ty_t src)
{
    return dst == src || (dst == STY_DNUM && src == STY_WNUM);
}

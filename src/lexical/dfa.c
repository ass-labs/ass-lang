#include "lexical/dfa.h"

static bool is_digit(uint8_t c) { return c >= '0' && c <= '9'; }

static bool is_ident_start(uint8_t c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_ident_char(uint8_t c) { return is_ident_start(c) || is_digit(c); }

/// Characters allowed after a backslash in string and char literals.
static bool is_escape(uint8_t c)
{
    switch (c)
    {
    case 'n': case 't': case 'r': case '0':
    case '\\': case '\'': case '"':
        return true;
    default:
        return false;
    }
}

/// Leaving a keyword path: any other identifier character continues a plain
/// identifier (`ifx`, `whilst`), anything else ends the token.
static dfa_state_t ident_step(uint8_t c)
{
    return is_ident_char(c) ? DFA_IDENT : DFA_DEAD;
}

/// Follows a keyword path on `expected`, or leaves it.
static dfa_state_t kw_step(uint8_t c, uint8_t expected, dfa_state_t next)
{
    return c == expected ? next : ident_step(c);
}

static dfa_state_t start_step(uint8_t c)
{
    switch (c)
    {
    case ' ': case '\t': case '\r': case '\n':
        return DFA_WS;

    // First letters of keywords and reserved words
    case 'w': return DFA_W;
    case 'd': return DFA_D;
    case 'c': return DFA_C;
    case 'b': return DFA_B;
    case 'i': return DFA_I;
    case 'e': return DFA_E;
    case 'r': return DFA_R;
    case 'A': return DFA_UA;
    case 'O': return DFA_UO;
    case 'N': return DFA_UN;
    case 'T': return DFA_UT;
    case 'F': return DFA_UF;
    case 'a': return DFA_A;
    case 'l': return DFA_L;
    case 's': return DFA_S;

    case '"': return DFA_STR_BODY;
    case '\'': return DFA_CHAR_OPEN;

    case '+': return DFA_PLUS;
    case '-': return DFA_MINUS;
    case '*': return DFA_STAR;
    case '/': return DFA_SLASH;
    case '%': return DFA_PERCENT;
    case '=': return DFA_EQ;
    case '!': return DFA_BANG;
    case '<': return DFA_LT;
    case '>': return DFA_GT;
    case '.': return DFA_DOT;
    case ',': return DFA_COMMA;
    case ';': return DFA_SEMI_COLON;
    case '{': return DFA_LBRACE;
    case '}': return DFA_RBRACE;
    case '(': return DFA_LPAREN;
    case ')': return DFA_RPAREN;
    case '[': return DFA_LBRACKET;
    case ']': return DFA_RBRACKET;

    case '$': return DFA_DOLLAR;
    case '@': return DFA_BLK_OPEN_AT;

    default:
        if (is_ident_start(c))
            return DFA_IDENT;
        if (is_digit(c))
            return DFA_INT;
        return DFA_DEAD;
    }
}

/// `wnum` and `dnum` branch into their sized variants (`wnum8`, `wnum16`, ...).
static dfa_state_t num_kw_step(uint8_t c, dfa_state_t s8, dfa_state_t s1,
                               dfa_state_t s3, dfa_state_t s6)
{
    switch (c)
    {
    case '8': return s8;
    case '1': return s1;
    case '3': return s3;
    case '6': return s6;
    default:  return ident_step(c);
    }
}

dfa_state_t dfa_step(dfa_state_t state, uint8_t c)
{
    switch (state)
    {
    case DFA_DEAD:
        return DFA_DEAD;

    case DFA_START:
        return start_step(c);

    case DFA_WS:
        switch (c)
        {
        case ' ': case '\t': case '\r': case '\n': return DFA_WS;
        default: return DFA_DEAD;
        }

    case DFA_IDENT:
        return ident_step(c);

    // w -> wnum, while, write
    case DFA_W:
        switch (c)
        {
        case 'n': return DFA_WN;
        case 'h': return DFA_WH;
        case 'r': return DFA_WR;
        default:  return ident_step(c);
        }
    case DFA_WN:    return kw_step(c, 'u', DFA_WNU);
    case DFA_WNU:   return kw_step(c, 'm', DFA_WNUM);
    case DFA_WNUM:
        return num_kw_step(c, DFA_WNUM8, DFA_WNUM1, DFA_WNUM3, DFA_WNUM6);
    case DFA_WNUM1: return kw_step(c, '6', DFA_WNUM16);
    case DFA_WNUM3: return kw_step(c, '2', DFA_WNUM32);
    case DFA_WNUM6: return kw_step(c, '4', DFA_WNUM64);
    case DFA_WH:    return kw_step(c, 'i', DFA_WHI);
    case DFA_WHI:   return kw_step(c, 'l', DFA_WHIL);
    case DFA_WHIL:  return kw_step(c, 'e', DFA_WHILE);
    case DFA_WR:    return kw_step(c, 'i', DFA_WRI);
    case DFA_WRI:   return kw_step(c, 't', DFA_WRIT);
    case DFA_WRIT:  return kw_step(c, 'e', DFA_WRITE);

    // d -> dnum
    case DFA_D:     return kw_step(c, 'n', DFA_DN);
    case DFA_DN:    return kw_step(c, 'u', DFA_DNU);
    case DFA_DNU:   return kw_step(c, 'm', DFA_DNUM);
    case DFA_DNUM:
        return num_kw_step(c, DFA_DNUM8, DFA_DNUM1, DFA_DNUM3, DFA_DNUM6);
    case DFA_DNUM1: return kw_step(c, '6', DFA_DNUM16);
    case DFA_DNUM3: return kw_step(c, '2', DFA_DNUM32);
    case DFA_DNUM6: return kw_step(c, '4', DFA_DNUM64);

    case DFA_C:     return kw_step(c, 'h', DFA_CH);
    case DFA_CH:    return kw_step(c, 'a', DFA_CHA);
    case DFA_CHA:   return kw_step(c, 'r', DFA_CHAR);

    case DFA_B:     return kw_step(c, 'o', DFA_BO);
    case DFA_BO:    return kw_step(c, 'o', DFA_BOO);
    case DFA_BOO:   return kw_step(c, 'l', DFA_BOOL);

    case DFA_I:     return kw_step(c, 'f', DFA_IF);

    case DFA_E:     return kw_step(c, 'l', DFA_EL);
    case DFA_EL:    return kw_step(c, 's', DFA_ELS);
    case DFA_ELS:   return kw_step(c, 'e', DFA_ELSE);

    case DFA_R:     return kw_step(c, 'e', DFA_RE);
    case DFA_RE:    return kw_step(c, 'a', DFA_REA);
    case DFA_REA:   return kw_step(c, 'd', DFA_READ);

    case DFA_UA:    return kw_step(c, 'N', DFA_UAN);
    case DFA_UAN:   return kw_step(c, 'D', DFA_UAND);

    case DFA_UO:    return kw_step(c, 'R', DFA_UOR);

    case DFA_UN:    return kw_step(c, 'O', DFA_UNO);
    case DFA_UNO:   return kw_step(c, 'T', DFA_UNOT);

    case DFA_UT:    return kw_step(c, 'r', DFA_UTR);
    case DFA_UTR:   return kw_step(c, 'u', DFA_UTRU);
    case DFA_UTRU:  return kw_step(c, 'e', DFA_UTRUE);

    case DFA_UF:    return kw_step(c, 'a', DFA_UFA);
    case DFA_UFA:   return kw_step(c, 'l', DFA_UFAL);
    case DFA_UFAL:  return kw_step(c, 's', DFA_UFALS);
    case DFA_UFALS: return kw_step(c, 'e', DFA_UFALSE);

    case DFA_A:     return kw_step(c, 'u', DFA_AU);
    case DFA_AU:    return kw_step(c, 't', DFA_AUT);
    case DFA_AUT:   return kw_step(c, 'o', DFA_AUTO);

    case DFA_L:     return kw_step(c, 'e', DFA_LE);
    case DFA_LE:    return kw_step(c, 't', DFA_LET);

    case DFA_S:     return kw_step(c, 't', DFA_ST);
    case DFA_ST:    return kw_step(c, 'r', DFA_STR);

    // Complete keywords and reserved words only continue as identifiers.
    case DFA_WNUM8: case DFA_WNUM16: case DFA_WNUM32: case DFA_WNUM64:
    case DFA_DNUM8: case DFA_DNUM16: case DFA_DNUM32: case DFA_DNUM64:
    case DFA_WHILE: case DFA_WRITE: case DFA_CHAR: case DFA_BOOL:
    case DFA_IF: case DFA_ELSE: case DFA_READ: case DFA_UAND: case DFA_UOR:
    case DFA_UNOT: case DFA_UTRUE: case DFA_UFALSE: case DFA_AUTO:
    case DFA_LET: case DFA_STR:
        return ident_step(c);

    case DFA_INT:
        if (is_digit(c))
            return DFA_INT;
        if (c == '.')
            return DFA_FRAC_SEP;
        return is_ident_start(c) ? DFA_BAD_IDENT : DFA_DEAD;

    case DFA_FRAC_SEP:
        return is_digit(c) ? DFA_FLOAT : DFA_DEAD;

    case DFA_FLOAT:
        if (is_digit(c))
            return DFA_FLOAT;
        return is_ident_start(c) ? DFA_BAD_IDENT : DFA_DEAD;

    case DFA_BAD_IDENT:
        return is_ident_char(c) ? DFA_BAD_IDENT : DFA_DEAD;

    // <char> is any byte except \", \\ and a newline.
    case DFA_STR_BODY:
        switch (c)
        {
        case '\n': return DFA_DEAD;
        case '\\': return DFA_STR_ESC;
        case '"':  return DFA_STR_END;
        default:   return DFA_STR_BODY;
        }
    case DFA_STR_ESC:
        return is_escape(c) ? DFA_STR_BODY : DFA_DEAD;

    // <char> is any byte except \', \\ and a newline.
    case DFA_CHAR_OPEN:
        switch (c)
        {
        case '\n': case '\'': return DFA_DEAD;
        case '\\': return DFA_CHAR_ESC;
        default:   return DFA_CHAR_BODY;
        }
    case DFA_CHAR_ESC:
        return is_escape(c) ? DFA_CHAR_BODY : DFA_DEAD;
    case DFA_CHAR_BODY:
        return c == '\'' ? DFA_CHAR_END : DFA_DEAD;

    case DFA_PLUS:  return c == '=' ? DFA_PLUS_EQ : DFA_DEAD;
    case DFA_MINUS: return c == '=' ? DFA_MINUS_EQ : DFA_DEAD;
    case DFA_STAR:  return c == '=' ? DFA_STAR_EQ : DFA_DEAD;
    case DFA_SLASH:
        switch (c)
        {
        case '/': return DFA_SLASH_SLASH;
        case '=': return DFA_SLASH_EQ;
        default:  return DFA_DEAD;
        }
    case DFA_SLASH_SLASH: return c == '=' ? DFA_SLASH_SLASH_EQ : DFA_DEAD;
    case DFA_EQ:    return c == '=' ? DFA_EQ_EQ : DFA_DEAD;
    case DFA_BANG:  return c == '=' ? DFA_BANG_EQ : DFA_DEAD;
    case DFA_LT:    return c == '=' ? DFA_LT_EQ : DFA_DEAD;
    case DFA_GT:    return c == '=' ? DFA_GT_EQ : DFA_DEAD;

    // Line endings are left to the whitespace state.
    case DFA_DOLLAR:
        return c == '$' ? DFA_LINE_COMMENT : DFA_DEAD;
    case DFA_LINE_COMMENT:
        return c == '\r' || c == '\n' ? DFA_DEAD : DFA_LINE_COMMENT;

    // The body may span lines and ends at the first closing `@$$`.
    case DFA_BLK_OPEN_AT:
        return c == '$' ? DFA_BLK_OPEN_DOLLAR : DFA_DEAD;
    case DFA_BLK_OPEN_DOLLAR:
        return c == '$' ? DFA_BLK_BODY : DFA_DEAD;
    case DFA_BLK_BODY:
        return c == '@' ? DFA_BLK_CLOSE_AT : DFA_BLK_BODY;
    case DFA_BLK_CLOSE_AT:
        switch (c)
        {
        case '@': return DFA_BLK_CLOSE_AT;
        case '$': return DFA_BLK_CLOSE_DOLLAR;
        default:  return DFA_BLK_BODY;
        }
    case DFA_BLK_CLOSE_DOLLAR:
        switch (c)
        {
        case '@': return DFA_BLK_CLOSE_AT;
        case '$': return DFA_BLK_END;
        default:  return DFA_BLK_BODY;
        }

    // Final states of fixed tokens have no outgoing transitions.
    default:
        return DFA_DEAD;
    }
}

bool dfa_accepts(dfa_state_t state, token_kind_t *kind)
{
    switch (state)
    {
    case DFA_WS: *kind = TOK_WHITESPACE; return true;

    // Identifiers, and keyword prefixes that are not complete words
    case DFA_IDENT:
    case DFA_W: case DFA_WN: case DFA_WNU:
    case DFA_WNUM1: case DFA_WNUM3: case DFA_WNUM6:
    case DFA_WH: case DFA_WHI: case DFA_WHIL:
    case DFA_WR: case DFA_WRI: case DFA_WRIT:
    case DFA_D: case DFA_DN: case DFA_DNU:
    case DFA_DNUM1: case DFA_DNUM3: case DFA_DNUM6:
    case DFA_C: case DFA_CH: case DFA_CHA:
    case DFA_B: case DFA_BO: case DFA_BOO:
    case DFA_I:
    case DFA_E: case DFA_EL: case DFA_ELS:
    case DFA_R: case DFA_RE: case DFA_REA:
    case DFA_UA: case DFA_UAN:
    case DFA_UO:
    case DFA_UN: case DFA_UNO:
    case DFA_UT: case DFA_UTR: case DFA_UTRU:
    case DFA_UF: case DFA_UFA: case DFA_UFAL: case DFA_UFALS:
    case DFA_A: case DFA_AU: case DFA_AUT:
    case DFA_L: case DFA_LE:
    case DFA_S: case DFA_ST:
        *kind = TOK_IDENT; return true;

    case DFA_WNUM:   *kind = TOK_KW_WNUM;   return true;
    case DFA_DNUM:   *kind = TOK_KW_DNUM;   return true;
    case DFA_CHAR:   *kind = TOK_KW_CHAR;   return true;
    case DFA_BOOL:   *kind = TOK_KW_BOOL;   return true;
    case DFA_IF:     *kind = TOK_KW_IF;     return true;
    case DFA_ELSE:   *kind = TOK_KW_ELSE;   return true;
    case DFA_WHILE:  *kind = TOK_KW_WHILE;  return true;
    case DFA_WRITE:  *kind = TOK_KW_WRITE;  return true;
    case DFA_READ:   *kind = TOK_KW_READ;   return true;
    case DFA_UAND:   *kind = TOK_KW_AND;    return true;
    case DFA_UOR:    *kind = TOK_KW_OR;     return true;
    case DFA_UNOT:   *kind = TOK_KW_NOT;    return true;
    case DFA_UTRUE:  *kind = TOK_KW_TRUE;   return true;
    case DFA_UFALSE: *kind = TOK_KW_FALSE;  return true;

    case DFA_WNUM8:  *kind = TOK_RW_WNUM8;  return true;
    case DFA_WNUM16: *kind = TOK_RW_WNUM16; return true;
    case DFA_WNUM32: *kind = TOK_RW_WNUM32; return true;
    case DFA_WNUM64: *kind = TOK_RW_WNUM64; return true;
    case DFA_DNUM8:  *kind = TOK_RW_DNUM8;  return true;
    case DFA_DNUM16: *kind = TOK_RW_DNUM16; return true;
    case DFA_DNUM32: *kind = TOK_RW_DNUM32; return true;
    case DFA_DNUM64: *kind = TOK_RW_DNUM64; return true;
    case DFA_AUTO:   *kind = TOK_RW_AUTO;   return true;
    case DFA_LET:    *kind = TOK_RW_LET;    return true;
    case DFA_STR:    *kind = TOK_RW_STR;    return true;

    case DFA_INT:       *kind = TOK_INT;     return true;
    case DFA_FLOAT:     *kind = TOK_FLOAT;   return true;
    case DFA_BAD_IDENT: *kind = TOK_UNKNOWN; return true;
    case DFA_STR_END:   *kind = TOK_STR;     return true;
    case DFA_CHAR_END:  *kind = TOK_CHAR;    return true;

    case DFA_PLUS:            *kind = TOK_PLUS;            return true;
    case DFA_PLUS_EQ:         *kind = TOK_PLUS_EQ;         return true;
    case DFA_MINUS:           *kind = TOK_MINUS;           return true;
    case DFA_MINUS_EQ:        *kind = TOK_MINUS_EQ;        return true;
    case DFA_STAR:            *kind = TOK_STAR;            return true;
    case DFA_STAR_EQ:         *kind = TOK_STAR_EQ;         return true;
    case DFA_SLASH:           *kind = TOK_SLASH;           return true;
    case DFA_SLASH_EQ:        *kind = TOK_SLASH_EQ;        return true;
    case DFA_SLASH_SLASH:     *kind = TOK_SLASH_SLASH;     return true;
    case DFA_SLASH_SLASH_EQ:  *kind = TOK_SLASH_SLASH_EQ;  return true;
    case DFA_PERCENT:         *kind = TOK_PERCENT;         return true;
    case DFA_EQ:              *kind = TOK_EQ;              return true;
    case DFA_EQ_EQ:           *kind = TOK_EQ_EQ;           return true;
    case DFA_BANG_EQ:         *kind = TOK_BANG_EQ;         return true;
    case DFA_LT:              *kind = TOK_LT;              return true;
    case DFA_LT_EQ:           *kind = TOK_LT_EQ;           return true;
    case DFA_GT:              *kind = TOK_GT;              return true;
    case DFA_GT_EQ:           *kind = TOK_GT_EQ;           return true;
    case DFA_DOT:             *kind = TOK_DOT;             return true;
    case DFA_COMMA:           *kind = TOK_COMMA;           return true;
    case DFA_SEMI_COLON:      *kind = TOK_SEMI_COLON;      return true;
    case DFA_LBRACE:          *kind = TOK_LBRACE;          return true;
    case DFA_RBRACE:          *kind = TOK_RBRACE;          return true;
    case DFA_LPAREN:          *kind = TOK_LPAREN;          return true;
    case DFA_RPAREN:          *kind = TOK_RPAREN;          return true;
    case DFA_LBRACKET:        *kind = TOK_LBRACKET;        return true;
    case DFA_RBRACKET:        *kind = TOK_RBRACKET;        return true;

    case DFA_LINE_COMMENT: *kind = TOK_LINE_COMMENT;  return true;
    case DFA_BLK_END:      *kind = TOK_BLOCK_COMMENT; return true;

    default:
        return false;
    }
}

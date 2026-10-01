#ifndef __SPAN_H__
#define __SPAN_H__

#include "spreadsheet/cell.h"

typedef struct
{
    cell_pos_t pos;
    uint16_t offset, len;
} span_t;

#endif

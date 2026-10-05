#include <stdarg.h>
#include <stdlib.h>

#include "utils/control.h"
#include "utils/log.h"

void terminate(uint32_t code, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    vprintln(fmt, args);
    va_end(args);
    exit(code);
}

#ifndef __UTILS_CONTROL_H__
#define __UTILS_CONTROL_H__

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "utils/log.h"

#define loop while (1)

_Noreturn void terminate(uint32_t code, char const *fmt, ...);

#define panic(fmt, ...)                                                        \
    {                                                                          \
        eprint("program panicked at (%s:%d): ", __FILE__, __LINE__);           \
        fprintf(stderr, fmt "\n" __VA_OPT__(, ) __VA_ARGS__);                  \
        __builtin_trap();                                                      \
    }

#endif

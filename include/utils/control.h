#ifndef __UTILS_CONTROL_H__
#define __UTILS_CONTROL_H__

#include <stdint.h>

#define loop while (1)

_Noreturn void terminate(uint32_t code, char const *fmt, ...);

_Noreturn void panic(char const *fmt, ...);

#endif

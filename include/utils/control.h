#ifndef __UTILS_CONTROL_H__
#define __UTILS_CONTROL_H__

#include <stdint.h>

#define loop while (1)

[[noreturn]]
void terminate(uint32_t code, char const *fmt, ...);

[[noreturn]]
void panic(char const *fmt, ...);

#endif

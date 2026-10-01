#ifndef __UTILS_GUARD_H__
#define __UTILS_GUARD_H__

#define assert_(expr, fmt, ...)                                                \
    do                                                                         \
    {                                                                          \
        if (!(expr))                                                           \
            panic(fmt, __VA_ARGS__);                                           \
    } while (0);

#define todo(fmt, ...)                                                         \
    {                                                                          \
        panic("not yet implemented: " fmt __VA_OPT__(, ) __VA_ARGS__);         \
    }

#endif

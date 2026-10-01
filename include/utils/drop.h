#ifndef __UTILS_DROP_H__
#define __UTILS_DROP_H__

#define drop(f) __attribute__((cleanup(f)))

#endif

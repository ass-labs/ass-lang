#ifndef __ITER_ITER_H__
#define __ITER_ITER_H__

#define iter_new(T) T##_iter_new
#define decl_iter_new(T) iter(T) iter_new(T)(T const *data, size_t n)
#define def_iter_new(T)                                                        \
    decl_iter_new(T) { return (iter(T)){.start = data, .end = data + n - 1}; }

#define iter_backward(T) T##_iter_backward
#define decl_iter_backward(T) iter(T) iter_backward(T)(T const *data, size_t n)
#define def_iter_backward(T)                                                   \
    decl_iter_backward(T)                                                      \
    {                                                                          \
        return (iter(T)){.start = data + n - 1, .end = data};                  \
    }

#define iter_rev(T) T##_iter_rev
#define decl_iter_rev(T) iter(T) iter_rev(T)(iter(T) * iter)
#define def_iter_rev(T)                                                        \
    decl_iter_rev(T)                                                           \
    {                                                                          \
        return (iter(T)){.start = iter->end, .end = iter->start};              \
    }

#define declare_iter(T)                                                        \
    typedef struct                                                             \
    {                                                                          \
        T const *start, *end;                                                  \
    } iter(T);                                                                 \
    decl_iter_new(T);                                                          \
    decl_iter_backward(T);                                                     \
    decl_iter_rev(T);

#define define_iter(T)                                                         \
    def_iter_new(T);                                                           \
    def_iter_backward(T);                                                      \
    def_iter_rev(T);

#define iter(T) T##_iter_t

#define iter_dir(iter) (ssize_t)(iter.start < iter.end ? 1 : -1)
#define iterate(T, iter)                                                       \
    for (T const *ptr = iter.start; ptr != iter.end + iter_dir(iter);          \
         ptr += iter_dir(iter))

#endif

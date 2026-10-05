#ifndef __CORE_COLLECTION_SLICE_H__
#define __CORE_COLLECTION_SLICE_H__

#define view(T) T##_vec_view

#define decl_view(T) slice(T) view(T)(vec(T) * data, size_t offset, size_t n)

#define def_view(T)                                                            \
    decl_view(T) { return (slice(T)){data->data + offset, n}; }

#define declare_slice(T)                                                       \
    typedef struct                                                             \
    {                                                                          \
        T const *data;                                                         \
        size_t n;                                                              \
    } T##_slice_t;                                                             \
    decl_view(T);

#define define_slice(T) def_view(T);

#define slice(T) T##_slice_t

#endif

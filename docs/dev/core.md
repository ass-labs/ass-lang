# `core` Documentation

## `core/collection/`

### `vec.h`

The pre-built implementation of `vector`s or dynamic arrays.

Example usage:

```c
declare_vec(int); // Declaration of the type `vec(int)`
                  // (or `int_vec_t` after macro expansion)
define_vec(int);  // Implementation of the function helper
                  // declarations from `declare_vec(int)`

int main()
{
    vec(int) nums; // `nums`'s type is a vector of integers
    for (size_t i = 0; i < nums.len; ++i)
    {
        println("nums[%zu]: %d", i, nums.data[i]);
    }

    return 0;
}
```

Notice that `declare_vec(int)` (or `declare_vec(T)` where `T` can be any type, and `int` in this case) is required, along with the implementation of its declaration `define_vec(int)` (or `define_vec(T)` where `T` can be any type), as `declare_vec(T)` is typically used for creating signatures in header files including the vector structure, and `define_vec(T)` adds their corresponding implementations. Without the two prior steps, `vec(T)` or any vector helper functions for that specific type will not be usable or will not exist at all.

#### Vector Fundamentals

#### Initialization

```c
declare_vec(int);
define_vec(int);

int main()
{
    vec(int) nums;
    return 0;
}
```

Leaving this as is will trigger an error:

```zsh
src/main.c: In function ‘main’:
src/main.c:N:O: error: ‘nums.data’ is used uninitialized [-Werror=uninitialized]
  O |     println("nums.data: %p", nums.data);
      |     ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
src/main.c:N:M: note: ‘nums’ declared here
  N |     vec(int) nums;
      |              ^~~~
cc1: all warnings being treated as errors
```

The variable `nums` is currently uninitialized and all warnings are treated as errors as per the configuration.

Fixing this with zero initialization will work, however, its internal data is still uninitialized and will require proper allocation.

```c
    vec(int) nums = {0};
    println("nums.data: %p", nums.data); // nums.data: (nil)
```

Initialization requires more than just the declaration. The instance must be properly initialized with the `vec_new(T)` function.

```c
    vec(int) nums;
    vec_new(int)(&nums, NULL, 8);

    println("nums.data: %p", nums.data); // nums.data: 0x[some_address_etc]
```

The signature of `vec_new(T)` is `void (vec(T) *vec, void (*el_destr)(T*), size_t cap)`, where:

- `vec` - is a pointer to the vector to be initialized.
- `el_destr` - is the general destructor/free function for the elements of the vector.
  _NOTE: More about destructors or free functions will be covered later, currently, simply think of it as the function which is responsible for cleaning up the memory used for the specific allocation._
- `cap` - is the initial capacity of the vector. The capacity is what determines how much elements can the vector currently handle.
  _NOTE: Reallocation and capacity expansion is handled by the vector helper functions and performs the appropriate reallocations when necessary._

The provided arguments:

- `&nums` (for `vec`) - The pointer to the vector to be initialized, in this case, a pointer to `nums`.
- `NULL` (for `el_destr`) - The element destructor is left as `NULL` as integers typically do not require any deallocation or explicit clean-up. (_NOTE: This will require an actual value for specific types which require explicit clean-up_)
- `8` (for `cap`) - The initial capacity or initial number of elements that the vector will be able to accommodate. (_NOTE: This can be any number, and this does not necessarily mean that the vector will only be able to hold this much elements, it allows for expansion._)

#### Deallocation/Clean-up

Using tools such as `valgrind` allows for the program's memory allocations to be tracked, and with the program below:

```c
declare_vec(int);
define_vec(int);

int main()
{
    vec(int) nums;
    vec_new(int)(&nums, NULL, 8);

    println("nums.data: %p", nums.data);

    return 0;
}
```

It shows that a memory leak has been detected, and requires clean-up:

```zsh
==3290== HEAP SUMMARY:
==3290==     in use at exit: 32 bytes in 1 blocks
==3290==   total heap usage: 2 allocs, 1 frees, 1,056 bytes allocated
==3290==
==3290== LEAK SUMMARY:
==3290==    definitely lost: 32 bytes in 1 blocks
==3290==    indirectly lost: 0 bytes in 0 blocks
==3290==      possibly lost: 0 bytes in 0 blocks
==3290==    still reachable: 0 bytes in 0 blocks
==3290==         suppressed: 0 bytes in 0 blocks
==3290== Rerun with --leak-check=full to see details of leaked memory
==3290==
==3290== For lists of detected and suppressed errors, rerun with: -s
==3290== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

Memory leaks occur when there are segments of memory in which is no longer accessible to the running program due to its inavailability or occupancy. `C` and some other programming languages do not have an automatic memory allocation tracking which explains its speed. However, this also means that the user of the language or the programmer is required to track all "heap" allocations or what is typically referred to as "dynamic allocation". Failing to free or clean a chunk of memory that is no longer of use will result to that chunk of memory being unusable as long as the program is running. To avoid this, a memory clean-up is performed after the usage of the allocated memory.

In order to perform clean-ups with this specific vector system, `vec_free(T)` must be used on the vector instance created.

```c
    vec(int) nums;
    vec_new(int)(&nums, NULL, 8);

    println("nums.data: %p", nums.data);
    vec_free(int)(&nums); // Frees the memory used
```

Valgrind heap summary:

```zsh
==3363== HEAP SUMMARY:
==3363==     in use at exit: 0 bytes in 0 blocks
==3363==   total heap usage: 2 allocs, 2 frees, 1,056 bytes allocated
==3363==
==3363== All heap blocks were freed -- no leaks are possible
==3363==
==3363== For lists of detected and suppressed errors, rerun with: -s
==3363== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

Inconveniently, this is a necessary part of programs which require dynamic allocation. Conveniently, variable declarations have an exclusive feature in `gnu11` which allows the clean-up function to be bound to the variable using attributes. For simplicity, the `utils/drop.h` contains the `drop` macro which is used for this:

```c
    drop(vec_free(int)) vec(int) nums;
    vec_new(int)(&nums, NULL, 8);

    println("nums.data: %p", nums.data);
```

Through this, the program is cleaner and no longer requires as to which line might be the variable must be freed.

#### Vector Iterators

Conveniently, vectors can be iterated through the use of iterators.

```c
    drop(vec_free(int)) vec(int) nums;
    vec_new(int)(&nums, NULL, 8);

    iter(int) nums_iter = vec_iter(int)(&nums);
```

Iterators do not require any clean-up or deallocation functions as they are read-only "views" or "slices" to a specific container. Meaning, the data within iterators are only "borrowed" from the original collection of data, which in this is `nums`.

`vec_iter(T)(vec(T) *vec)`

- `vec` - The vector to "borrow" from

#### Iterator Iteration

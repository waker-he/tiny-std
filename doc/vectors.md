# vectors

- [Common](#common)
- [`small_vector<T, N>`](#small_vectort-n)
- [`vector<T>`](#vectort)
- [`inplace_vector<T, N>`](#inplace_vectort-n)

## Common

- implemented using `vector_mixin`
    - [`vector_mixin.cpp`](../module/vectors/vector_mixin.cpp)
    - it provides
        - member types: `iterator`, `const_iterator`, `size_type`
        - methods: `operator[]`, `begin`, `end`, `empty`
    - it requires
        - methods: `data`, `size`
<!-- - applied attribute [`[[clang::trivial_abi]]`](https://clang.llvm.org/docs/AttributeReference.html#trivial-abi) -->
- __relocate__ (noexcept if `__is_trivially_relocatable(T)` or `std::is_trivially_move_constructible_v<T>`)
    - if `__is_trivially_relocatable(T)`:
        - perform `memcpy`
    - else
        - uninitialized_move_if_noexcept
        - `std::destroy`


## `small_vector<T, N>`

- [`small_vector.cppm`](../module/vectors/small_vector.cppm)
- not allocator-aware
- usage in `constexpr` context:
    - if `N == 0`:
        - we are using dynamically allocated memory for all objects, with `constexpr std::allocator<T>::allocate`, we can use it in `constexpr` for any type `T`
    - else if `T` is trivially default constructible and destructible
        - used `T[N]` for storage
        - can be used in `constexpr`
    - else
        - use `alignas(T) char[N * sizeof(T)]`
        - will need `reinterpret_cast` to access element
        - `reinterpret_cast` cannot be used in `constexpr` context since it need to be ensured that `constexpr` context does not have __undefined behavior__
- moved-from `small_vector` is defined to be empty
- __assignment: using copy-swap idiom__
    - this handle the problem of following data structure
        ```cpp
        struct Node { std::vector<Node> children; };
        ```
- __swap:__ kind of complicated to implement to get both exception safety and performance
    - if constexpr N == 0
        - swap all members
    - else
        - if both have objects on heap:
            - then swap all members
        - else
            - if constexpr T is nothrow_relocatable
                - relocate lhs to temp
                - relocate rhs to lhs
                - relocate temp to rhs
            - else
                - allocate heap memory temp1, temp2 for lhs and rhs (might throw)
                - relocate lhs to temp1 (might throw)
                - relocate rhs to temp2 (might throw)
                - set lhs.m_data to temp2
                - set rhs.m_data to temp1
                - swap size of lhs and rhs
                - capacity is determined by heap memory allocated
- with this swap implementation, `capacity() == N` does not guarantee the objects are stored in buffer

## `vector<T>`

```cpp
template <typename T>
using vector = small_vector<T, 0>;
```

## `inplace_vector<T, N>`

- comparisons to `vector`:
    - pros
        - do not need branch that checks whether `size() == capacity()` when `emplace_back`
        - allocating elements on stack is faster and more cache-friendly
    - cons
        - more expensive for move operations
- different from `array<T, N>`
    - `array` will start the lifetime for each of its elemnts when constructed, while `inplace_vector` won't
- `T` is required to be `nothrow_relocatable` for operations `swap` and assignment, __reasoning:__
    - if `T` is not `nothrow_relocatable`, for `swap`, we have:
        - relocate from *this to temp (might throw, but fine here)
        - relocate from other to *this
            - might throw
            - if throw, we need to relocate temp back to *this, this relocation can throw again

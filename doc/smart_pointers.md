# smart pointers

## `unique_ptr<T>`

- code: [unique_ptr.cppm](../module/smart_pointers/unique_ptr.cppm)
- type requirements
    - `T`: not an array
- to make moving `unique_ptr` as cheap as raw pointer:
    - applied attribute [`[[clang::trivial_abi]]`](https://clang.llvm.org/docs/AttributeReference.html#trivial-abi)


## `shared_ptr<T>` and `weak_ptr<T>`

- code:
    - [shared_ptr.cppm](../module/smart_pointers/shared_ptr.cppm)
    - [weak_ptr.cppm](../module/smart_pointers/weak_ptr.cppm)
- type requirements
    - `T`: not an array
- applied attribute [`[[clang::trivial_abi]]`](https://clang.llvm.org/docs/AttributeReference.html#trivial-abi)
- control block implementation:
    - `control_block`
        - __abstract base class__, enables:
            - alias pointers
            - `make_shared` to allocate control block and object together
            - custom allocator and deleter (not implemented in `tinystd::shared_ptr<T>`)
        - __destructor and `delete_obj` are `protected`__: control block is reponsible for deleting itself
        - __non-static data members__ 
            - shared_count: number of `shared_ptr` referencing it
            - weak_count: number of `weak_ptr` referencing it + (shared_cnt != 0)
        - __atomic operations__ on reference counts:
            - refer to [a well-explained answer on relaxed atomic usage for `shared_ptr` on stack overflow](https://stackoverflow.com/questions/48124031/stdmemory-order-relaxed-atomicity-with-respect-to-the-same-atomic-variable/48148318#48148318)
            - __increment__:
                - used `std::memory_order_relaxed` for both weak_count and shared_count
                    - once control block is constructed, the increment order does not matter
                    - invoking of copy constructors always have a `happens-before` relationship
                - `increment_shared_if_not_zero` uses CAS operation, this method will be called by `weak_ptr`
            - __decrement__:
                - shared_count
                    ```cpp
                    if (shared_count.fetch_sub(1, std::memory_order_release) == 1) {
                        std::atomic_thread_fence(std::memory_order_acquire);
                        delete_obj();
                    }
                    ```
                    - technically, only the thread which modifies the shared object need to do a __release store__, and only the last thread that decrement the `shared_count` need an __acquire load__
                    - but thread actions differ in each run, so we used `std::memory_order_release` for all decrements
                - weak_count
                    - since it is only used to delete control block and control block is trivially destructible, we can also use `std::memory_order_relaxed`
                    - need to do acquire-release if we enable custom allocator and deleter that are not trivially destructible
- consequence of enabling alias pointers:
    - `control_block_with_ptr` needs to store the pointer to the actual object managed
    - `weak_ptr<T>` needs to store `T*` in addition to `control_block*`

## `enalble_shared_from_this`

- code: [enable_shared_from_this.cppm](../module/smart_pointers/enable_shared_from_this.cppm)
- inheriting publicly will enable an object to take part in its own lifetime management when managed by `shared_ptr`
- constructing a `shared_ptr` for an object that is already managed by another `shared_ptr` (through constructor `shared_ptr(T* ptr)`) is undefined behavior
- implementation uses C++23 __deducing this__ to replace CRTP
- non-static data member:
    - `control_block*`

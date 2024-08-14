# smart pointers

## `unique_ptr<T>`

- type requirements
    - `T`: not an array
- to make moving `unique_ptr` as cheap as raw pointer:
    - applied attribute [`[[clang::trivial_abi]]`](https://clang.llvm.org/docs/AttributeReference.html#trivial-abi)
    - does not check for self-assigment
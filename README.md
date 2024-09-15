# tiny-std

- __Reinventing the wheel for learning__: implement some chosen functionalities from C++ standard library and Boost C++ libraries for practice purpose
- __Proof of concept__: not feature-complete, not heavily optimized
- __Compiler__: clang++ 19.1.0
- Use __C++20 module__ to organize code
- __Dependencies__
    - [boost-ext/ut](https://github.com/boost-ext/ut): unit test, managed by CPM
    - [nanobench](https://github.com/martinus/nanobench): benchmark, managed by CPM
    - Boost 1.81+: manually installed

## Implemented

- [vectors](./doc/vectors.md)
    - `vector`
    - `small_vector` (Boost)
    - `inplace_vector` (C++26) 
- [smart pointers](./doc/smart_pointers.md)
    - `unique_ptr` (C++11)
    - `shared_ptr` (C++11)
    - `weak_ptr` (C++11)
    - `enable_shared_from_this` (C++11)
    - `atomic_shared_ptr` (C++20)
- [`span`](./doc/span.md) (C++20)
- [`waitfree_spsc_queue`](./doc/waitfree_spsc_queue.md) (Boost `boost::lock_free:spsc_queue`)
- [`hazard_pointer`](./doc/hazard_pointer.md) (C++26)
- [`any`](./doc/any.md) (C++17)

## Learnings

- general
    - rule of 5
    - exception safety
    - constrain template instantiation with C++20 concepts
    - deeper understandings of the implemented functionalities
- vectors
    - object lifetime
    - `constexpr` restriction
    - optimized relocate operation
    - use __deducing this__ to implement mixin class and write overloaded member functions as a single member function template
- smart pointers
    - C++ memory model and relaxed atomic
- `waitfree_spsc_queue`/`hazard_pointer`/`atomic_shared_ptr`
    - lock free programming and optimizations
- `any`
    - type erasure

## Plan to Implement

- `variant` (C++17): learn value-semantics-based implementation of Visitor design pattern
- `optional` (C++17): deepen understanding of monadic operations
- `generator` (C++23): gain experience programming with C++20 coroutine
- `tuple` (C++11): practice template metaprogramming
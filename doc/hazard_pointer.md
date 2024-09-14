## [Index](../README.md)

# `hazard_pointer`

- commented code: [hazard_pointer.cppm](../module/hazard_pointer.cppm)

## Implementation Requirements

1. Bounded number of unreclaimed objects
2. High throughput

## Key Design Choices and Trade-offs

### Intrusive vs Non-intrusive

This implementation opts for a non-intrusive approach, unlike the C++26 `hazard_pointer_obj_base`.

#### Intrusive Approach (not chosen)
- Pros:
  - Greater implementation flexibility (e.g., adding a `next` pointer for linked list implementation of retire list)
  - Enables global and local retire lists, allowing one thread to own multiple hazard pointers while ensuring bounded unreclaimed objects
- Cons:
  - Objects incur cost even when not using hazard pointers
  - Creates a dependency: objects need to know about hazard pointers

### Maximum Number of Hazard Pointers per Thread

This implementation limits each thread to owning at most one hazard pointer.

#### Arbitrary Number (not chosen)
- Pros:
  - Greater usage flexibility
- Cons:
  - Reduced performance due to longer, more expensive-to-scan hazard pointer lists
  - Complicates ensuring bounded unreclaimed objects without making it intrusive

### Retired List Location

Retired lists are stored in the `hp_slot` rather than using global and thread-local retired lists.

#### Global and Thread-local Retired Lists (not chosen)
- Pros:
  - Allows arbitrary number of hazard pointers per thread
- Cons:
  - Requires intrusive approach (to add `next` pointer)
  - Reduced performance due to contention on global list and inability to directly reuse retired objects across threads

## Implementation Details

### Template Parameter `T`

- Retire list of type `T` only needs to check the protected list of type `T`
- A thread can own multiple hazard pointers of different types

### `hazard_pointer` Class

- Templated with parameter `T`, unlike C++26 `hazard_pointer`
- Represents ownership of a particular hazard pointer slot
- RAII: Resets protection in destructor if protecting any pointer

### `hp_slot`

- Acquired for a thread on first interaction with hazard pointer functionalities (`protect`/`retire`)
- Released when thread exits

### Performance

- Time complexity of `retire()` is amortized O(1) in terms of P (number of threads)
- Cleanup process guarantees reclamation of at least P objects for every 2P retires

### Limitations

- Each thread can only own one hazard pointer for a given pointer type `T*` (multiple ownership leads to undefined behavior)
- Moving ownership of a hazard pointer across threads is undefined behavior

## References

- [P2530R3: Hazard Pointers for C++26](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2530r3.pdf)
- [Daniel Anderson's implementation](https://github.com/DanielLiamAnderson/atomic_shared_ptr/blob/master/include/parlay/details/hazard_pointers.hpp)
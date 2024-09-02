## [Index](../README.md)

# `waitfree_spsc_queue`

- commented code: [waitfree_spsc_queue.cppm](../module/waitfree_spsc_queue.cppm)
- bounded
- optimizations
    - replace MOD op with AND op by using power of 2 as capacity of ring buffer
    - double-checked empty/full using cached cursors
    - load atomic cursors up-front to avoid redundant atomic load operations
- reference
    - [Single Producer Single Consumer Lock-free FIFO From the Ground Up - Charles Frasch - CppCon 2023](https://www.youtube.com/watch?v=K3P_Lmq6pw0&t=2s)

## Benchmark

- benchmark code: [benchmark_waitfree_spsc_queue.cpp](../benchmark/benchmark_waitfree_spsc_queue.cpp)
- hardware:
    - **CPU:** Intel(R) Core(TM) i9-9980HK CPU @ 2.40GHz
    - **VM Cores:** 4
    - **Threads per Core:** 1
    - **Memory:** 7.7Gi total (912Mi used, 5.1Gi free)
    - **Disk:** 63G total (27G used, 33G free, 46% use) on /dev/sda3
    - **OS:** Ubuntu 22.04.2 LTS
    - **Kernel:** 6.8.0-40-generic
- results:

| relative | ns/op        | op/s   | err% | total | benchmark                        |
|----------|--------------|--------|------|-------|----------------------------------|
| 100.0%   | 6,592,705.91 | 151.68 | 1.2% | 39.42 | `boost::lockfree::spsc_queue`    |
| 304.0%   | 2,168,312.96 | 461.19 | 1.6% | 13.04 | `tinystd::waitfree_spsc_queue`   |



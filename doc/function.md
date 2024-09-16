## [Index](../README.md)

# `function`

- code: [function.cppm](../module/function.cppm)
- use small size optimization and manual dispatch

## benchmark against libc++'s std::function

| Benchmark | Custom function (ns/op) | std::function (ns/op) | Performance Difference |
|-----------|------------------------:|----------------------:|----------------------:|
| Small callable | 2.48 | 2.40 | -3.3% |
| Lambda | 1.47 | 1.41 | -4.3% |
| Large callable | 1.41 | 1.43 | +1.4% |
| Multi-arg | 0.28 | 1.76 | +84.1% |


1. **Small callable and Lambda**: The custom `function` implementation performs slightly slower (3-4%) than `std::function` for small callables and lambdas. This difference is minimal and may be within the margin of error.
2. **Large callable**: For large callables, the custom `function` shows a marginal improvement (1.4%) over `std::function`. This suggests that both implementations handle large objects similarly, possibly using heap allocation.
3. **Multi-argument function**: The custom `function` significantly outperforms `std::function` for multi-argument functions, being about 84% faster. This is a substantial improvement and could be a key advantage of the custom implementation.
4. **Overall Performance**: The custom `function` implementation is competitive with `std::function` across most scenarios, with performance ranging from slightly slower to significantly faster.
5. **Optimization Opportunities**: The slight underperformance in small callable and lambda scenarios might be areas for potential optimization in the custom implementation.

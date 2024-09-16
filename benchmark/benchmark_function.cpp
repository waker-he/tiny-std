#include <nanobench.h>

import std;
import tinystd;

// Ensure we're using the custom function
using tinystd::function;

// Small callable
int small_callable() {
    return 42;
}

// Larger callable
class LargeCallable {
    std::array<int, 100> data{};
public:
    int operator()() const { return data[0]; }
};

// Function with multiple arguments
int multi_arg_func(int a, int b, int c, int d) {
    return a + b + c + d;
}

template<typename Func>
void run_benchmark(ankerl::nanobench::Bench& bench, const char* name, Func&& func) {
    bench.run(name, [&] {
        ankerl::nanobench::doNotOptimizeAway(func());
    });
}

int main() {
    ankerl::nanobench::Bench bench;

    // Benchmark small callable
    run_benchmark(bench, "Custom function (small callable)", function<int()>(small_callable));
    run_benchmark(bench, "std::function (small callable)", std::function<int()>(small_callable));

    // Benchmark lambda
    auto lambda = []() { return 42; };
    run_benchmark(bench, "Custom function (lambda)", function<int()>(lambda));
    run_benchmark(bench, "std::function (lambda)", std::function<int()>(lambda));

    // Benchmark larger callable
    run_benchmark(bench, "Custom function (large callable)", function<int()>(LargeCallable()));
    run_benchmark(bench, "std::function (large callable)", std::function<int()>(LargeCallable()));

    // Benchmark multi-argument function
    auto multi_arg_lambda = [](int a, int b, int c, int d) { return a + b + c + d; };
    bench.run("Custom function (multi-arg)", [&] {
        function<int(int,int,int,int)> f(multi_arg_lambda);
        ankerl::nanobench::doNotOptimizeAway(f(1, 2, 3, 4));
    });
    bench.run("std::function (multi-arg)", [&] {
        std::function<int(int,int,int,int)> f(multi_arg_lambda);
        ankerl::nanobench::doNotOptimizeAway(f(1, 2, 3, 4));
    });

    return 0;
}
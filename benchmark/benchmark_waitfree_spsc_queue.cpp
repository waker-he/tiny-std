#include <boost/lockfree/spsc_queue.hpp>
#include <nanobench.h>

import std;
import tinystd;

template <class Queue>
void
pop(Queue& q) noexcept
{
    if constexpr (std::same_as<Queue, boost::lockfree::spsc_queue<int>>)
    {
        int i;
        while (!q.pop(i));
    }
    else { while (!q.pop().has_value()); }
}

template <class Queue>
void
push(Queue& q, int i)
{
    if constexpr (std::same_as<Queue, boost::lockfree::spsc_queue<int>>)
    {
        while (!q.push(i));
    }
    else { while (!q.emplace(i)); }
}

template <class Queue>
void
benchmark_queue(ankerl::nanobench::Bench& bench, std::string const & name)
{
    int const q_cap = 1048576;
    int const items = 1000000;
    Queue     q(q_cap);
    bench.minEpochIterations(100).run(
        name,
        [&]
        {
            std::jthread producer(
                [&]
                {
                    for (int i = 0; i < items; ++i) { push(q, i); }
                }
            );
            std::jthread consumer(
                [&]
                {
                    for (int i = 0; i < items; ++i) { pop(q); }
                }
            );
        }
    );
}

int
main()
{
    ankerl::nanobench::Bench bench;
    bench.relative(true);
    benchmark_queue<boost::lockfree::spsc_queue<int>>(
        bench, "boost::lockfree::spsc_queue"
    );
    benchmark_queue<tinystd::waitfree_spsc_queue<int>>(
        bench, "tinystd::waitfree_spsc_queue"
    );
}
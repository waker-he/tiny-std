#include <boost/smart_ptr/atomic_shared_ptr.hpp>
#include <nanobench.h>

import tinystd;
import std;

template <typename T>
class lockfree_stack
{
private:
    struct node
    {
        T                         data;
        tinystd::shared_ptr<node> next; // every node is managed by shared_ptr
    };
    tinystd::atomic_shared_ptr<node> head;

public:
    void
    push(T const & data)
    {
        tinystd::shared_ptr<node> new_node{
            new node{data, head.load(std::memory_order_relaxed)}
        };
        while (!head.compare_exchange_weak(
            new_node->next,
            std::move(new_node),
            std::memory_order_release,
            std::memory_order_relaxed
        ));
    }

    std::optional<T>
    pop() noexcept
    {
        tinystd::shared_ptr<node> old_head =
            head.load(std::memory_order_relaxed);
        while (old_head
               && !head.compare_exchange_weak(
                   old_head,
                   std::move(old_head->next),
                   std::memory_order_acquire,
                   std::memory_order_relaxed
               ));
        std::optional<T> item;
        if (old_head) item.emplace(std::move(old_head->data));
        return item;
    }
};

template <typename T>
class lockfree_stack_boost
{
private:
    struct node
    {
        T                       data;
        boost::shared_ptr<node> next; // every node is managed by shared_ptr
    };
    boost::atomic_shared_ptr<node> head;

public:
    void
    push(T const & data)
    {
        boost::shared_ptr<node> new_node{
            new node{data, head.load(std::memory_order_relaxed)}
        };
        while (!head.compare_exchange_weak(new_node->next, new_node));
    }

    std::optional<T>
    pop() noexcept
    {
        boost::shared_ptr<node> old_head = head.load(std::memory_order_relaxed);
        while (old_head && !head.compare_exchange_weak(old_head, old_head->next)
        );
        std::optional<T> item;
        if (old_head) item.emplace(std::move(old_head->data));
        return item;
    }
};


template <typename Stack, bool is_producer>
void
worker(Stack& stack, size_t operations)
{
    for (size_t i = 0; i < operations; ++i)
    {
        if constexpr (is_producer) { stack.push(i); }
        else { while (!stack.pop()); }
    }
}

template <typename Stack>
void
run_benchmark(ankerl::nanobench::Bench& bench, std::string const & name)
{
    size_t const num_threads           = std::thread::hardware_concurrency();
    size_t const operations_per_thread = 1000000;

    bench.minEpochIterations(1).run(
        name,
        [&]
        {
            Stack                    stack;
            std::vector<std::thread> threads;
            threads.reserve(num_threads);

            for (size_t i = 0; i < num_threads / 2; ++i)
            {
                threads.emplace_back(
                    worker<Stack, true>, std::ref(stack), operations_per_thread
                );
            }
            for (size_t i = 0; i < num_threads / 2; ++i)
            {
                threads.emplace_back(
                    worker<Stack, false>, std::ref(stack), operations_per_thread
                );
            }

            for (auto& thread : threads) { thread.join(); }
        }
    );
}

int
main()
{
    ankerl::nanobench::Bench bench;

    bench.relative(true);
    run_benchmark<lockfree_stack_boost<int>>(
        bench, "Boost Atomic Shared Ptr Stack"
    );
    run_benchmark<lockfree_stack<int>>(
        bench, "TinySTD Atomic Shared Ptr Stack"
    );

    return 0;
}
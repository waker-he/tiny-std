// this has to be included otherwise simd instruction cannot be found
#include <boost/unordered/unordered_flat_set.hpp>

import tinystd;
import std;


template <typename T>
class LockFreeStack
{
private:
    struct Node
    {
        T     data;
        Node* next;

        Node(T const & value) : data(value), next(nullptr) {}
    };

    std::atomic<Node*> head;

public:
    LockFreeStack() : head(nullptr) {}

    ~LockFreeStack()
    {
        while (pop()) {}
    }

    void
    push(T const & value)
    {
        Node* new_node = new Node(value);
        new_node->next = head.load(std::memory_order_relaxed);
        while (!head.compare_exchange_weak(
            new_node->next,
            new_node,
            std::memory_order_release,
            std::memory_order_relaxed
        ))
        {
            // Retry if CAS fails
        }
    }

    std::optional<T>
    pop()
    {
        tinystd::hazard_pointer<Node> hp = tinystd::make_hazard_pointer<Node>();
        Node*                         old_head = hp.protect(head);

        while (old_head)
        {
            if (head.compare_exchange_strong(
                    old_head,
                    old_head->next,
                    std::memory_order_acquire,
                    std::memory_order_relaxed
                ))
            {
                T result = old_head->data;
                hp.reset_protection();
                tinystd::hazard_pointer<Node>::retire(old_head);
                return result;
            }
            old_head = hp.protect(head);
        }

        return std::nullopt;
    }
};

// Test function
void
test_lock_free_stack()
{
    LockFreeStack<int> stack;

    // Push some elements
    for (int i = 0; i < 100; ++i) { stack.push(i); }

    // Pop and verify elements
    for (int i = 99; i >= 0; --i)
    {
        auto value = stack.pop();
        if (!value || *value != i)
        {
            throw std::runtime_error("Stack test failed!");
        }
    }

    // Stack should be empty now
    if (stack.pop()) { throw std::runtime_error("Stack should be empty!"); }

    std::cout << "Lock-free stack test passed successfully!" << std::endl;
}

void
push_task(
    LockFreeStack<int>& stack, int start, int end, std::atomic<int>& push_count
)
{
    for (int i = start; i < end; ++i)
    {
        stack.push(i);
        ++push_count;
    }
}

void
pop_task(
    LockFreeStack<int>& stack,
    std::atomic<int>&   pop_count,
    std::atomic<bool>&  stop_flag
)
{
    std::random_device              rd;
    std::mt19937                    gen(rd());
    std::uniform_int_distribution<> dis(1, 10);

    while (!stop_flag.load(std::memory_order_relaxed))
    {
        auto value = stack.pop();
        if (value) { ++pop_count; }
        else
        {
            // If pop fails, sleep for a short random time
            std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
        }
    }

    // Drain the stack
    while (stack.pop()) { ++pop_count; }
}

void
run_concurrent_test(int num_threads, int operations_per_thread)
{
    LockFreeStack<int> stack;
    std::atomic<int>   push_count(0);
    std::atomic<int>   pop_count(0);
    std::atomic<bool>  stop_flag(false);

    std::vector<std::thread> threads;

    // Create push threads
    for (int i = 0; i < num_threads / 2; ++i)
    {
        threads.emplace_back(
            push_task,
            std::ref(stack),
            i * operations_per_thread,
            (i + 1) * operations_per_thread,
            std::ref(push_count)
        );
    }

    // Create pop threads
    for (int i = 0; i < num_threads / 2; ++i)
    {
        threads.emplace_back(
            pop_task, std::ref(stack), std::ref(pop_count), std::ref(stop_flag)
        );
    }

    // Let the test run for a while
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Signal pop threads to stop
    stop_flag.store(true, std::memory_order_relaxed);

    // Wait for all threads to finish
    for (auto& thread : threads) { thread.join(); }

    std::cout << "Push count: " << push_count << std::endl;
    std::cout << "Pop count: " << pop_count << std::endl;

    // Verify that all pushed elements were popped
    assert(push_count == pop_count && "Mismatch between push and pop counts");
    std::cout << "Test passed successfully!" << std::endl;
}

int
main()
{
    test_lock_free_stack();
    run_concurrent_test(
        8, 100000
    ); // 8 threads, 100,000 operations per push thread
}
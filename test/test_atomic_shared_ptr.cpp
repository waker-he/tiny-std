#include <boost/ut.hpp>

import tinystd;
import std;

using namespace boost::ut;
using namespace std::literals;
using namespace tinystd;

struct TestObject
{
    int                     value;
    static std::atomic<int> instance_count;
    TestObject(int v) : value(v) { instance_count++; }
    ~TestObject() { instance_count--; }
};
std::atomic<int> TestObject::instance_count(0);

suite<"atomic_shared_ptr"> atomic_shared_ptr_test = []
{
    "default constructor"_test = []
    {
        atomic_shared_ptr<TestObject> asp;
        expect(asp.load().use_count() == 0_l);
    };

    "constructor with shared_ptr"_test = []
    {
        shared_ptr<TestObject>        sp = make_shared<TestObject>(42);
        atomic_shared_ptr<TestObject> asp(std::move(sp));
        expect(asp.load()->value == 42_i);
        expect(sp.use_count() == 0_l);
    };

    "load and store"_test = []
    {
        atomic_shared_ptr<TestObject> asp;
        {
            shared_ptr<TestObject> sp1 = make_shared<TestObject>(10);
            asp.store(sp1);
            shared_ptr<TestObject> sp2 = asp.load();
            expect(sp2->value == 10_i);
            expect(sp1.use_count() == 3_l);
        }
        expect(TestObject::instance_count.load() == 1_i);
    };

    "exchange"_test = []
    {
        atomic_shared_ptr<TestObject> asp(make_shared<TestObject>(10));
        shared_ptr<TestObject>        sp1 = make_shared<TestObject>(20);
        shared_ptr<TestObject>        old = asp.exchange(sp1);
        expect(old->value == 10_i);
        expect(asp.load()->value == 20_i);
    };

    "compare_exchange_weak success"_test = []
    {
        atomic_shared_ptr<TestObject> asp(make_shared<TestObject>(10));
        shared_ptr<TestObject>        expected = asp.load();
        shared_ptr<TestObject>        desired  = make_shared<TestObject>(20);
        bool success = asp.compare_exchange_weak(expected, std::move(desired));
        expect(success);
        expect(asp.load()->value == 20_i);
        expect(expected->value == 10_i);
    };

    "compare_exchange_weak failure"_test = []
    {
        atomic_shared_ptr<TestObject> asp(make_shared<TestObject>(10));
        shared_ptr<TestObject>        expected = make_shared<TestObject>(5);
        shared_ptr<TestObject>        desired  = make_shared<TestObject>(20);
        bool success = asp.compare_exchange_weak(expected, std::move(desired));
        expect(!success);
        expect(asp.load()->value == 10_i);
        expect(expected->value == 10_i);
    };

    "compare_exchange_strong success"_test = []
    {
        atomic_shared_ptr<TestObject> asp(make_shared<TestObject>(10));
        shared_ptr<TestObject>        expected = asp.load();
        shared_ptr<TestObject>        desired  = make_shared<TestObject>(20);
        bool                          success =
            asp.compare_exchange_strong(expected, std::move(desired));
        expect(success);
        expect(asp.load()->value == 20_i);
        expect(expected->value == 10_i);
    };

    "compare_exchange_strong failure"_test = []
    {
        atomic_shared_ptr<TestObject> asp(make_shared<TestObject>(10));
        shared_ptr<TestObject>        expected = make_shared<TestObject>(5);
        shared_ptr<TestObject>        desired  = make_shared<TestObject>(20);
        bool                          success =
            asp.compare_exchange_strong(expected, std::move(desired));
        expect(!success);
        expect(asp.load()->value == 10_i);
        expect(expected->value == 10_i);
    };

    "concurrent operations"_test = []
    {
        atomic_shared_ptr<TestObject> asp(make_shared<TestObject>(0));
        expect(fatal(TestObject::instance_count.load() == 1_i));
        std::atomic<int> success_count(0);

        constexpr int NUM_THREADS = 4;
        constexpr int ITERATIONS  = 10000;

        std::vector<std::jthread> threads;

        for (int i = 0; i < NUM_THREADS; ++i)
        {
            threads.emplace_back(
                [&]
                {
                    for (int j = 0; j < ITERATIONS; ++j)
                    {
                        shared_ptr<TestObject> expected = asp.load();
                        shared_ptr<TestObject> desired =
                            make_shared<TestObject>(expected->value + 1);
                        if (asp.compare_exchange_weak(
                                expected, std::move(desired)
                            ))
                        {
                            success_count.fetch_add(
                                1, std::memory_order_relaxed
                            );
                        }
                    }
                }
            );
        }

        threads.clear(); // Join all threads

        expect(asp.load()->value == success_count.load());
        expect(TestObject::instance_count.load() == 1_i);
    };
};

template <typename T>
    requires std::is_nothrow_move_constructible_v<T>
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

suite<"lockfree_stack"> lockfree_stack_test = []
{
    "push and pop"_test = []
    {
        lockfree_stack<int> stack;

        stack.push(1);
        stack.push(2);
        stack.push(3);

        expect(stack.pop().value() == 3_i);
        expect(stack.pop().value() == 2_i);
        expect(stack.pop().value() == 1_i);
        expect(!stack.pop().has_value());
    };

    "empty stack"_test = []
    {
        lockfree_stack<int> stack;
        expect(!stack.pop().has_value());
    };

    "push and pop multiple types"_test = []
    {
        lockfree_stack<std::string> stack;

        stack.push("hello");
        stack.push("world");

        expect(eq(stack.pop().value(), "world"s));
        expect(eq(stack.pop().value(), "hello"s));
    };

    "concurrent push and pop"_test = []
    {
        lockfree_stack<int> stack;
        std::atomic<int>    sum{0};
        std::atomic<int>    push_count{0};
        std::atomic<int>    pop_count{0};

        constexpr int NUM_THREADS = 4;
        constexpr int ITERATIONS  = 10000;

        std::vector<std::jthread> threads;

        for (int i = 0; i < NUM_THREADS; ++i)
        {
            threads.emplace_back(
                [&]
                {
                    for (int j = 0; j < ITERATIONS; ++j)
                    {
                        if (j % 2 == 0)
                        {
                            stack.push(j);
                            push_count.fetch_add(1, std::memory_order_relaxed);
                        }
                        else
                        {
                            auto val = stack.pop();
                            if (val.has_value())
                            {
                                sum.fetch_add(
                                    val.value(), std::memory_order_relaxed
                                );
                                pop_count.fetch_add(
                                    1, std::memory_order_relaxed
                                );
                            }
                        }
                    }
                }
            );
        }

        threads.clear(); // Join all threads

        expect(eq(push_count.load(), pop_count.load()))
            << "Push and pop counts should be equal";
        expect(sum.load() < (ITERATIONS * NUM_THREADS / 2 * (ITERATIONS - 1)))
            << "Sum should be less than the maximum possible sum";
    };
};

int
main()
{
}

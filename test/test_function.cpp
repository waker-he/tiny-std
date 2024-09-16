#include <boost/ut.hpp>
import tinystd;
import std;

using namespace tinystd;
using namespace boost::ut;
using namespace std::literals;

int global_value = 0;

void
free_function(int x)
{
    global_value = x;
}

struct Functor
{
    void
    operator()(int x) const
    {
        value = x;
    }
    mutable int value = 0;
};

class TestClass
{
public:
    void
    member_function(int x)
    {
        value = x;
    }
    int value = 0;
};

struct ResourceManager
{
    int*        resource_count;
    mutable int call_count = 0;

    ResourceManager(int* r) noexcept : resource_count(r)
    {
        ++(*resource_count);
    }
    ResourceManager(ResourceManager const & other) noexcept
        : resource_count(other.resource_count)
    {
        ++(*resource_count);
    }
    ~ResourceManager() noexcept { --(*resource_count); }

    void
    operator()(int x) const
    {
        call_count = x;
    }
};

suite<"function"> function_test = []
{
    "default constructor"_test = []
    {
        function<void()> f;
        expect(not f);
    };

    "nullptr constructor"_test = []
    {
        function<void()> f(nullptr);
        expect(not f);
    };

    "function pointer"_test = []
    {
        function<void(int)> f(free_function);
        expect(f);
        f(42);
        expect(global_value == 42_i);
    };

    "non-capturing lambda"_test = []
    {
        function<int()> f([] { return 42; });
        expect(f);
        expect(f() == 42_i);
    };

    "functor"_test = []
    {
        Functor             functor;
        function<void(int)> f(functor);
        expect(f);
        f(42);
        expect(functor.value == 0_i); // The original functor is unchanged
        // We can't directly access the copied functor inside f
    };

    "copy constructor"_test = []
    {
        function<int()> f1([] { return 42; });
        function<int()> f2(f1);
        expect(f2);
        expect(f2() == 42_i);
    };

    "move constructor"_test = []
    {
        function<int()> f1([] { return 42; });
        function<int()> f2(std::move(f1));
        expect(f2);
        expect(not f1);
        expect(f2() == 42_i);
    };

    "assignment operator"_test = []
    {
        function<int()> f1([] { return 42; });
        function<int()> f2;
        f2 = f1;
        expect(f2);
        expect(f2() == 42_i);
    };

    "nullptr assignment"_test = []
    {
        function<void()> f([] {});
        expect(f);
        f = nullptr;
        expect(not f);
    };

    "reset"_test = []
    {
        function<void()> f([] {});
        expect(f);
        f.reset();
        expect(not f);
    };

    "swap"_test = []
    {
        function<int()> f1([] { return 1; });
        function<int()> f2([] { return 2; });
        swap(f1, f2);
        expect(f1() == 2_i);
        expect(f2() == 1_i);
    };

    "small buffer optimization"_test = []
    {
        // This lambda should fit in the small buffer
        function<int()> f([] { return 42; });
        expect(f);
        expect(f() == 42_i);
    };

    "large callable"_test = []
    {
        // This functor should not fit in the small buffer
        struct LargeFunctor
        {
            std::array<int, 100> data{};
            int
            operator()() const
            {
                return data[0];
            }
        };
        function<int()> f(LargeFunctor{});
        expect(f);
        expect(f() == 0_i);
    };

    "member function pointer"_test = []
    {
        TestClass                       obj;
        function<void(TestClass&, int)> f(&TestClass::member_function);
        expect(f);
        f(obj, 42);
        expect(obj.value == 42_i);
    };

    "member function pointer with std::bind"_test = []
    {
        TestClass obj;
        auto      bound_func =
            std::bind(&TestClass::member_function, &obj, std::placeholders::_1);
        function<void(int)> f(bound_func);
        expect(f);
        f(42);
        expect(obj.value == 42_i);
    };

    "resource management"_test = []
    {
        int resource_count = 0;
        {
            function<void(int)> f{ResourceManager{&resource_count}};
            expect(resource_count == 1_i);
            f(42);
            function<void(int)> f2 = f;
            expect(resource_count == 2_i);
        }
        expect(resource_count == 0_i);
    };
};

int
main()
{
}
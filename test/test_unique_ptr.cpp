#include <cassert>

import std;
import tinystd;

struct S
{
    int* resources;
    constexpr S(int* r) noexcept : resources(r) { ++(*resources); }
    constexpr virtual ~S() noexcept { --(*resources); }
};

struct DeriveS : S
{
    constexpr DeriveS(int* r) noexcept : S(r) {}
};


consteval int
test()
{
    using namespace tinystd;

    int resources = 0;
    {
        unique_ptr<S> p1;
        assert(!p1);
        p1.reset(new S{&resources});
        assert(p1);

        unique_ptr<S> p2 = p1.release();
        assert(!p1);
        assert(p2);
        swap(p1, p2);
        assert(p1);
        assert(!p2);

        // move assignment
        p2 = std::move(p1);
        assert(!p1);
        assert(p2);

        unique_ptr<S> p3(std::move(p2));
        assert(!p2);
        assert(p3);
        p3 = nullptr;
        assert(!p3);

        unique_ptr<S> p4(make_unique<DeriveS>(&resources));
    }
    return resources;
}

static_assert(test() == 0);

int
main()
{
}
#include <boost/ut.hpp>
#include <cassert>

import tinystd;
import std;

struct [[clang::trivial_abi]] S
{
    inline static int resources = 0; // #resourcesAcquired - #resources Released
    inline static int count     = 0; // #constructed - #destoryed

    int resource;

    S() noexcept : resource(1)
    {
        ++resources;
        ++count;
    }

    ~S() noexcept
    {
        resources -= resource;
        --count;
    }

    S(S const & other) noexcept : resource(other.resource)
    {
        resources += resource;
        ++count;
    }

    S(S&& other) noexcept : resource(other.resource)
    {
        other.resource = 0;
        ++count;
    }

    S&
    operator=(S const & other) noexcept
    {
        resources -= resource;
        resource = other.resource;
        resources += resource;
        return *this;
    }

    S&
    operator=(S&& other) noexcept
    {
        resources -= resource;
        resource       = other.resource;
        other.resource = 0;
        return *this;
    }
};

static_assert(__is_trivially_relocatable(S));

template <class VectorInt>
consteval bool
test_vector_compile_time()
{
    VectorInt v;
    // assert(v.empty());
    v.emplace_back(3);
    v.emplace_back(1);
    v.emplace_back(7);
    v.emplace_back(5);
    assert(v[1] == 1);
    assert(v.size() == 4);

    std::ranges::sort(v);
    assert(v[0] == 1);
    assert(v[1] == 3);
    assert(v[2] == 5);
    assert(v[3] == 7);
    v.pop_back();
    assert(v.size() == 3);

    return true;
}

using namespace tinystd;
static_assert(sizeof(vector<int>) == 24ul);
static_assert(test_vector_compile_time<vector<int>>());
static_assert(test_vector_compile_time<small_vector<int, 2>>());
static_assert(test_vector_compile_time<inplace_vector<int, 5>>());


using namespace boost::ut;

suite<"vectors"> vectors = []
{
    "special member functions"_test = []<class VectorS>
    {
        boost::ut::log << reflection::type_name<VectorS>();
        {
            VectorS v1;
            v1.emplace_back();
            v1.emplace_back();

            boost::ut::log << "copy ctor...";
            auto v2(v1);
            expect(v1.size() == 2_ul);
            expect(v2.size() == 2_ul);

            boost::ut::log << "move ctor...";
            v2.emplace_back();
            auto v3(std::move(v2));
            expect(v2.size() == 0_ul);
            expect(v3.size() == 3_ul);

            boost::ut::log << "copy assignment...";
            v2 = v3;
            expect(v2.size() == 3_ul);

            boost::ut::log << "move assignment...\n";
            v3 = std::move(v2);
            expect(v2.size() == 0_ul);
            expect(v3.size() == 3_ul);
        }

        expect(fatal(S::resources == 0_ul))
            << "leaking " << S::resources << " resources";
        expect(fatal(S::count == 0_ul)) << "leaking " << S::count << "objects";
    } | std::tuple<small_vector<S, 2>, inplace_vector<S, 3>, vector<S>>{};
};


int
main()
{
}

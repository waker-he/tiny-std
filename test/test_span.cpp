#include <boost/ut.hpp>

import std;
import tinystd;

using namespace tinystd;
using namespace boost::ut;

// sizeof check
static_assert(sizeof(span<int>) == sizeof(void*) * 2);
static_assert(sizeof(span<int, 1>) == sizeof(void*));

suite<"span"> span_test = []
{
    "default constructor"_test = []
    {
        span<int> s;
        expect(s.size() == 0_u);
        expect(s.data() == nullptr);
        expect(s.empty());
    };

    "constructor with pointer and size"_test = []
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        span<int>        s(v.data(), v.size());
        expect(s.size() == 5_ul);
        expect(eq(s.data(), v.data()));
        expect(!s.empty());
    };

    "constructor with iterators"_test = []
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        span<int>        s(v.data(), v.data() + v.size());
        expect(s.size() == 5_ul);
        expect(eq(s.data(), v.data()));
    };

    "element access"_test = []
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        span<int>        s(v.data(), v.size());
        expect(s[0] == 1_i);
        expect(s[4] == 5_i);
    };

    "iterators"_test = []
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        span<int>        s(v.data(), v.size());
        expect(*s.begin() == 1_i);
        expect(*(s.end() - 1) == 5_i);
    };

    "subspan"_test = []
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        span<int>        s(v.data(), v.size());

        auto sub1 = s.subspan<1, 3>();
        expect(sub1.size() == 3_ul);
        expect(sub1[0] == 2_i);
        expect(sub1[2] == 4_i);

        auto sub2 = s.subspan(2, 2);
        expect(sub2.size() == 2_ul);
        expect(sub2[0] == 3_i);
        expect(sub2[1] == 4_i);
    };

    "first and last"_test = []
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        span<int>        s(v.data(), v.size());

        auto first3 = s.first<3>();
        expect(first3.size() == 3_ul);
        expect(first3[2] == 3_i);

        auto last2 = s.last<2>();
        expect(last2.size() == 2_ul);
        expect(last2[0] == 4_i);
        expect(last2[1] == 5_i);

        auto first2 = s.first(2);
        expect(first2.size() == 2_ul);
        expect(first2[1] == 2_i);

        auto last3 = s.last(3);
        expect(last3.size() == 3_ul);
        expect(last3[0] == 3_i);
    };

    "fixed extent"_test = []
    {
        std::array<int, 5> arr = {1, 2, 3, 4, 5};
        span<int, 5>       s(arr.data(), arr.size());
        expect(s.size() == 5_ul);
        expect(s.extent == 5_ul);
    };

    "dynamic extent"_test = []
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        span<int>        s(v.data(), v.size());
        expect(s.size() == 5_ul);
        expect(s.extent == dynamic_extent);
    };
};


int
main()
{
}
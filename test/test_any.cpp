#include <boost/ut.hpp>
#include <cassert>

import tinystd;
import std;

using namespace tinystd;
using namespace boost::ut;
using namespace std::literals;

struct TestType
{
    int value;
    TestType(int v) : value(v) {}
};

suite<"any"> any_test = []
{
    "default constructor"_test = []
    {
        any a;
        expect(not a.has_value());
    };

    "constructor with value"_test = []
    {
        any a(42);
        expect(a.has_value());
        expect(a.type() == typeid(int));
        expect(*any_cast<int>(&a) == 42_i);
    };

    "copy constructor"_test = []
    {
        any a1(42);
        any a2(a1);
        expect(a2.has_value());
        expect(a2.type() == typeid(int));
        expect(*any_cast<int>(&a2) == 42_i);
    };

    "move constructor"_test = []
    {
        any a1(42);
        any a2(std::move(a1));
        expect(a2.has_value());
        expect(a2.type() == typeid(int));
        expect(*any_cast<int>(&a2) == 42_i);
        expect(not a1.has_value());
    };

    "assignment operator"_test = []
    {
        any a1(42);
        any a2;
        a2 = a1;
        expect(a2.has_value());
        expect(a2.type() == typeid(int));
        expect(*any_cast<int>(&a2) == 42_i);
    };

    "emplace"_test = []
    {
        any   a;
        auto& ref = a.emplace<TestType>(10);
        expect(a.has_value());
        expect(a.type() == typeid(TestType));
        expect(any_cast<TestType>(&a)->value == 10_i);
        expect(ref.value == 10_i);
    };

    "reset"_test = []
    {
        any a(42);
        expect(a.has_value());
        a.reset();
        expect(not a.has_value());
    };

    "swap"_test = []
    {
        any a1(42);
        any a2(std::string("Hello"));
        swap(a1, a2);
        expect(a1.type() == typeid(std::string));
        expect(*any_cast<std::string>(&a1) == "Hello"s);
        expect(a2.type() == typeid(int));
        expect(*any_cast<int>(&a2) == 42_i);
    };

    "make_any"_test = []
    {
        auto a = make_any<std::vector<int>>(3, 42);
        expect(a.type() == typeid(std::vector<int>));
        auto* vec = any_cast<std::vector<int>>(&a);
        expect(vec != nullptr);
        expect(vec->size() == 3_u);
        expect((*vec)[0] == 42_i);
        expect((*vec)[1] == 42_i);
        expect((*vec)[2] == 42_i);
    };

    "any_cast"_test = []
    {
        any a(42);
        expect(*any_cast<int>(&a) == 42_i);
        expect(any_cast<double>(&a) == nullptr);
    };
};

consteval bool
test_any_constexpr()
{
    tinystd::any a1(42);
    assert(a1.has_value());
    assert(a1.type() == typeid(int));

    tinystd::any a2(3.14);
    assert(a2.has_value());
    assert(a2.type() == typeid(double));

    a2 = a1;
    assert(a2.type() == typeid(int));

    int* value_ptr = tinystd::any_cast<int>(&a2);
    assert(value_ptr != nullptr);
    assert(*value_ptr == 42);

    a1.reset();
    assert(!a1.has_value());

    return true;
}

// This will cause a compile-time error if test_any_constexpr() cannot be
// evaluated at compile-time
static_assert(test_any_constexpr());

int
main()
{
}
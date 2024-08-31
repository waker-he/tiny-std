#include <boost/ut.hpp>

import tinystd;
import std;

using namespace tinystd;
using namespace boost::ut;

struct Base
{
    inline static std::atomic<int> resources =
        0; // #resourcesAcquired - #resources Released
    Base() noexcept { resources.fetch_add(1, std::memory_order_relaxed); }
    virtual ~Base() = default;

    int i = 0;
};

struct Derive : Base
{
    ~Derive() override { resources.fetch_sub(1, std::memory_order_relaxed); }
};

suite<"shared_ptr"> shared_ptr_test = []
{
    "move and alias ctor"_test = []
    {
        {
            shared_ptr<Derive> sp1(new Derive{});         // regular ctor
            shared_ptr<Base>   sp2(std::move(sp1));       // move ctor
            expect(sp2->i == 0_i);                        // deref
            shared_ptr<int> sp3(std::move(sp2), &sp2->i); // alias
            expect(*sp3 == 0_i);                          // deref
            expect(sp3.use_count() == 1_l);
            expect(sp2.use_count() == 0_l);
            expect(sp1.use_count() == 0_l);
        }
        expect(fatal(Base::resources.load(std::memory_order_relaxed) == 0_i));
    };

    "make_shared"_test = []
    {
        {
            auto             sp1{make_shared<Derive>()};
            shared_ptr<Base> sp2(std::move(sp1));
            expect(sp2->i == 0_i);
            expect(neq(sp2, nullptr));
            shared_ptr<int> sp3(std::move(sp2), &sp2->i);
            expect(*sp3 == 0_i);
            expect(sp3.use_count() == 1_l);
            expect(sp2.use_count() == 0_l);
            expect(sp1.use_count() == 0_l);

            auto sp4{sp3};
            expect(sp3.use_count() == 2_l);
            expect(eq(sp3, sp4));
            auto sp5{std::move(sp3)};
            expect(neq(sp3, sp4));
            expect(sp3.use_count() == 0_l);
            expect(sp4.use_count() == 2_l);
            expect(sp5.use_count() == 2_l);
        }
        expect(fatal(Base::resources.load(std::memory_order_relaxed) == 0_i));
    };

    "assignment"_test = []
    {
        {
            shared_ptr<Derive> sp1(new Derive{});
            sp1 = sp1;
            expect(sp1.use_count() == 1_l);
            shared_ptr<Base> sp2(new Derive{});
            sp2 = sp1;
            expect(sp1.use_count() == 2_l);
            sp2 = sp1;
            expect(sp1.use_count() == 2_l);
            expect(sp2.use_count() == 2_l);

            auto up(make_unique<Derive>());
            sp2 = std::move(up);
            expect(sp2.use_count() == 1_l);
            expect(sp1.use_count() == 1_l);
        }
        expect(fatal(Base::resources.load(std::memory_order_relaxed) == 0_i));
    };

    "concurrent"_test = []
    {
        constexpr int NUM_THREADS = 100;

        {
            // shared_ptr must be declared before threads so that it is
            // destroyed before threads are joined
            auto                      sp0{make_shared<Derive>()};
            std::vector<std::jthread> threads;
            for (int i = 0; i < NUM_THREADS; ++i)
            {
                threads.emplace_back(
                    [](shared_ptr<Derive> const & sp0)
                    {
                        for (int i = 0; i < 10000; ++i)
                        {
                            auto sp1 = sp0;
                            auto sp2 = make_shared<Derive>();
                            sp1.swap(sp2);
                        }
                    },
                    std::ref(sp0)
                );
            }
        }
        expect(fatal(Base::resources.load(std::memory_order_relaxed) == 0_i));
    };
};


int
main()
{
}
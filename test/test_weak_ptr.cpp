#include <boost/ut.hpp>

import std;
import tinystd;

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

suite<"weak_ptr"> weak_ptr_test = []
{
    "default ctor"_test = []
    {
        weak_ptr<Base> wp;
        expect(wp.expired());
        expect(wp.use_count() == 0_l);
    };

    "conversion from/to shared_ptr"_test = []
    {
        {
            shared_ptr<Base> sp{make_shared<Derive>()};
            weak_ptr<Base>   wp(sp);
            auto             sp2 = wp.lock();
            expect(static_cast<bool>(sp2));
            expect(wp.use_count() == 2_l);

            sp.reset();
            sp2.reset();
            expect(wp.expired());
            expect(fatal(Base::resources.load(std::memory_order_relaxed) == 0_i)
            );
        }
        expect(fatal(Base::resources.load(std::memory_order_relaxed) == 0_i));
    };

    "copy/move ctor"_test = []
    {
        {
            auto             sp = make_shared<Derive>();
            weak_ptr<Derive> wp1(sp);

            weak_ptr<Derive> wp2(wp1); // Copy
            expect(wp2.use_count() == 1_l);

            weak_ptr<Base> wp3(std::move(wp2)); // Move
            expect(wp3.use_count() == 1_l);
            expect(wp2.expired());
        }
        expect(fatal(Base::resources.load(std::memory_order_relaxed) == 0_i));
    };

    "assignments"_test = []
    {
        {
            // assignments
            shared_ptr<Derive> sp1 = make_shared<Derive>();
            shared_ptr<Derive> sp2 = make_shared<Derive>();
            weak_ptr<Derive>   wp1(sp1);
            weak_ptr<Derive>   wp2(sp2);

            wp1 = wp2; // Assign from weak_ptr
            expect(wp1.use_count() == 1_l);

            wp1 = sp1; // Assign from shared_ptr
            expect(wp1.use_count() == 1_l);
        }
        expect(fatal(Base::resources.load(std::memory_order_relaxed) == 0_i));
    };

    "concurrent"_test = []
    {
        constexpr int    NUM_THREADS      = 100;
        std::atomic<int> successful_locks = 0;

        {
            auto             sp{make_shared<Derive>()};
            weak_ptr<Derive> wp = sp;

            using namespace std::literals::chrono_literals;
            std::vector<std::jthread> threads;
            for (int i = 0; i < NUM_THREADS; ++i)
            {
                threads.emplace_back(
                    [](weak_ptr<Derive> const & wp, std::atomic<int>& cnt_locks)
                    {
                        while (true)
                        {
                            if (auto sp = wp.lock(); !sp) { break; }
                            else
                            {
                                cnt_locks.fetch_add(
                                    1, std::memory_order_relaxed
                                );
                            }
                            std::this_thread::sleep_for(1ms);
                        }
                    },
                    std::ref(wp),
                    std::ref(successful_locks)
                );
            }

            std::this_thread::sleep_for(2s);
            sp.reset();
        }

        boost::ut::log << "Total successful locks: "
                       << successful_locks.load(std::memory_order_relaxed)
                       << '\n';
        expect(fatal(Base::resources.load(std::memory_order_relaxed) == 0_i));
    };
};


int
main()
{
}
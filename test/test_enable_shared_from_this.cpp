#include <boost/ut.hpp>

import std;
import tinystd;

using namespace tinystd;
using namespace boost::ut;

struct TestESFT : public enable_shared_from_this
{
    inline static int cnt   = 0;
    int               value = 42;

    TestESFT() noexcept { ++cnt; }
    ~TestESFT() noexcept { --cnt; }
};


suite<"enable_shared_from_this"> enable_shared_from_this_test = []
{
    "basic"_test = []
    {
        {
            auto sp1 = make_shared<TestESFT>();
            auto sp2 = sp1->shared_from_this();

            expect(eq(sp1.get(), sp2.get()));
            expect(sp2->value == 42_i);
            expect(sp1.use_count() == 2_l);

            auto wp = sp2->weak_from_this();
            expect(wp.lock().get() == sp1.get());

            expect(throws(
                []
                {
                    TestESFT t;
                    auto     sp = t.shared_from_this();
                }
            ));

            expect(throws(
                []
                {
                    TestESFT t;
                    auto     wp = t.weak_from_this();
                }
            ));
        }
        expect(fatal(TestESFT::cnt == 0_i));
    };
};


int
main()
{
}
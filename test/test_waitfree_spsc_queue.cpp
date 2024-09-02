#include <boost/ut.hpp>
#include <cassert>

import std;
import tinystd;

using namespace tinystd;
using namespace boost::ut;

suite<"waitfree_spsc_queue"> test_waitfree_spsc_queue = []
{
    "basic"_test = []
    {
        waitfree_spsc_queue<int> queue(1000);
        std::atomic<bool>        done(false);

        std::jthread producer(
            [&]
            {
                for (int i = 0; i < 10000; ++i)
                {
                    while (!queue.emplace(i)) { std::this_thread::yield(); }
                }
                done = true;
            }
        );

        int          consumed = 0;
        std::jthread consumer(
            [&]
            {
                while (true)
                {
                    if (auto popped = queue.pop(); popped.has_value())
                    {
                        expect(fatal(eq(*popped, consumed)));
                        ++consumed;
                    }
                    else
                    {
                        if (done) return;
                        std::this_thread::yield();
                    }
                }
            }
        );

        producer.join();
        consumer.join();

        expect(consumed == 10000_i);
    };
};

int
main()
{
}
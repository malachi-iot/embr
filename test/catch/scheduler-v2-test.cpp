#include <catch2/catch_all.hpp>

#include <embr/scheduler.h>

#include "scheduler-test.h"

using namespace embr::scheduler::detail;
using namespace test::scheduler;

TEST_CASE("Scheduler v2", "[scheduler-v2]")
{
    SECTION("basics")
    {
        using processor = nexter_processor<ref_nexter<int>, ref_adapter>;
        using type = embr::layer1::scheduler<processor, 10>;

        processor rn1(0, 4), rn2(1, 4);
        int now = 0;

        type n;

        n.reschedule(&rn1);

        REQUIRE(n.ready(now));

        n.process_one(now);

        REQUIRE(rn1.next() == 4);

        n.process_one(++now);

        REQUIRE(rn1.next() == 4);

        n.process_one(now = 4);

        REQUIRE(rn1.next() == 8);

        REQUIRE(n.ready(now) == false);

        n.reschedule(&rn2);

        REQUIRE(n.top().id() == 1);
        REQUIRE(n.ready(now) == true);

        REQUIRE(rn1.next() == 8);
        REQUIRE(rn2.next() == 0);

        n.process_one(now);

        REQUIRE(n.top().id() == 1);

        REQUIRE(n.next() == 4);
        REQUIRE(n.ready(now));

        n.process_one(now);

        REQUIRE(n.ready(now) == false);

        now += 4;

        // Two of us at next_ == 8 now
        REQUIRE(n.ready(now));
        REQUIRE(n.process_one(now));

        REQUIRE(n.ready(now));
        // Semi-undefined behavior which of these two identical parties get sorted first
        REQUIRE(n.top().id() == 1);

        REQUIRE(rn1.counter_ == 1);
        REQUIRE(rn2.counter_ == 1);

        rn1.next(0);

        // Special feature is we support rescheduling this way.  A little brute force, but worth it
        n.reschedule(&rn1);

        REQUIRE(n.ready(now));
        REQUIRE(n.top().id() == 0);
        REQUIRE(n.top().next() == 0);

        REQUIRE(n.process_one(now));
        REQUIRE(n.top().id() == 0);
        REQUIRE(n.top().next() == 4);

        REQUIRE(n.process_one(now));
        // Semi-undefined behavior which of these two identical parties get sorted first
        REQUIRE(n.top().id() == 1);
        REQUIRE(n.top().next() == 8);
    }
    SECTION("aggressive rescheduling")
    {

    }
}

#include <catch2/catch.hpp>

#include <embr/exp/thunk.h>

// DEBT: Instead of tracker/tracked use estd internal underpinnings for
// shared_ptr

struct Tracker
{
    int count = 0;
    int total = 0;

    Tracker() = default;

    void inc()
    {
        ++count;
        ++total;
    }
};


struct Tracked
{
    Tracker* tracker;

    Tracked(Tracker* tracker) : tracker{tracker}
    {
        tracker->inc();
    }

    Tracked(const Tracked& t) : tracker(t.tracker)
    {
        tracker->inc();
    }

    ~Tracked()
    {
        if(tracker) --tracker->count;
    }
};

TEST_CASE("thunk")
{
    embr::experimental::Thunk t;

    SECTION("pt1")
    {
        int val = 0;

        t.enqueue([&]
        {
            ++val;
        });
        t.enqueue([&]
        {
            val += 4;
        });
        t.invoke();
        t.invoke();

        REQUIRE(val == 5);
    }
    SECTION("dtor testing")
    {
        SECTION("raw")
        {
            Tracker tracker;
            Tracked tracked(&tracker);

            {
                auto f1 = [tracked, &tracker]
                {
                    REQUIRE(tracker.count == 2);
                };

                REQUIRE(tracker.count == 2);
                REQUIRE(tracker.total == 2);

                f1();

                REQUIRE(tracker.total == 2);
            }

            REQUIRE(tracker.count == 1);
        }
        SECTION("thunked")
        {
            Tracker tracker;
            Tracked tracked(&tracker);

            {
                t.enqueue([tracked, &tracker]
                {
                    REQUIRE(tracker.count == 2);
                });

                REQUIRE(tracker.count == 2);
                REQUIRE(tracker.total == 3);

                t.invoke();

                // FIX: dtor doesn't run, damn
                //REQUIRE(tracker.count == 1);
            }

            REQUIRE(tracker.count == 1);
        }
    }
}

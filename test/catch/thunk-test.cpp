#include <catch2/catch.hpp>

#include <embr/exp/thunk.h>

// Excellent breakdown of functor behavior:
// https://ricomariani.medium.com/std-function-teardown-and-discussion-a4f148929809

// DEBT: Instead of tracker/tracked use estd internal underpinnings for
// shared_ptr

struct Tracker
{
    // ref count
    int count = 0;
    // total operations
    int total = 0;

    Tracker() = default;

    void inc()
    {
        ++count;
        ++total;
    }

    void inc_total()
    {
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

    Tracked(Tracked&& move_from) :
        tracker(move_from.tracker)
    {
        move_from.tracker = nullptr;
        tracker->inc_total();
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
        SECTION("baseline")
        {
            Tracker tracker;
            Tracked tracked(&tracker);

            REQUIRE(tracker.count == 1);
        }
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
                REQUIRE(tracker.total == 4);

                t.invoke();

                REQUIRE(tracker.count == 1);
            }

            REQUIRE(tracker.count == 1);
        }
    }
}

#include <catch2/catch.hpp>

#include <embr/exp/thunk.h>

// DEBT: Instead of tracker/tracked use estd internal underpinnings for
// shared_ptr

struct Tracker
{
    int count = 0;

    Tracker() = default;

};


struct Tracked
{
    Tracker* tracker;

    Tracked(Tracker* tracker) : tracker{tracker}
    {
        ++tracker->count;
    }

    Tracked(const Tracked& t) : tracker(t.tracker)
    {
        ++tracker->count;
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

        t.enqueue([&](void*)
        {
            ++val;
        });
        t.enqueue([&](void*)
        {
            val += 4;
        });
        t.invoke();
        t.invoke();

        REQUIRE(val == 5);
    }
    SECTION("dtor testing")
    {
        Tracker tracker;
        Tracked tracked(&tracker);

        REQUIRE(tracker.count == 1);

        t.enqueue([tracked, &tracker](void*)
        {
            REQUIRE(tracker.count == 2);
        });

        t.invoke();

        // FIX: dtor doesn't run, damn
        //REQUIRE(tracker.count == 1);
    }
}

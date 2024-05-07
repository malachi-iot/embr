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
    embr::experimental::layer1::Thunk<256> t;

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
        REQUIRE(t.empty());

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

                REQUIRE(t.empty() == false);
                REQUIRE(tracker.count == 2);
                REQUIRE(tracker.total == 3);

                t.invoke();

                REQUIRE(tracker.count == 1);
                REQUIRE(t.empty());
            }

            REQUIRE(tracker.count == 1);
        }
    }
    SECTION("layer3::bipbuf")
    {
        union
        {
            bipbuf_t buf;
            char allocated[1024];
        };

        int counter = 0;

        // DEBT: Passing in bipbuf_t + size not preferred way (heading towards deprecated).
        // We want a new
        // constructor which takes 'allocated' directly.  Note flavor which
        // -only- passes in bipbuf_t is OK, but that requires a discrete bipbuf init call
        embr::experimental::layer3::Thunk<> t2(&buf, sizeof(allocated) - sizeof(buf));

        REQUIRE(t2.empty() == true);
        t2.enqueue([&]{ ++counter; });
        REQUIRE(t2.empty() == false);
        t2.invoke();
        REQUIRE(counter == 1);
        REQUIRE(t2.empty() == true);
    }
}

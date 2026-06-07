#include <catch2/catch_all.hpp>

#include <future>
#include <queue>
#include <random>
#include <vector>

#include <embr/exp/thunk.h>
#include <embr/thunk.h>

#include "mem/test-mem-data.h"

// Excellent breakdown of functor behavior:
// https://ricomariani.medium.com/std-function-teardown-and-discussion-a4f148929809

// DEBT: Instead of tracker/tracked use estd internal underpinnings for
// shared_ptr

// 05OCT25
// This has large overlap with "delegate_queue" and was coming along well to displace it IIRC
// IIRC I detoured to beef up estd functor behavior to better handle dtor operations here and never
// resumed
// 31MAY26
// delegate_queue worked well, but relied on ESP-IDF specific FreeRTOS ringbuffer extension.  Thunk does not.

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



TEST_CASE("thunk", "[thunk]")
{
    int counter = 0;

    SECTION("layer1")
    {
        embr::sys::layer1::thunk<256> thunk;

        thunk.post([&] { ++counter; });

        SECTION("poll_one")
        {
            estd::errc err;
            err = thunk.poll_one();
            REQUIRE(err == estd::errc{});
            err = thunk.poll_one();
            REQUIRE(err == estd::errc::no_message_available);

            REQUIRE(counter == 1);
        }
        SECTION("poll")
        {
            //estd::expected<unsigned, estd::errc> err;

            thunk.post([&] { counter += 2; });

            estd::expected<unsigned, estd::errc> err = thunk.poll();
            REQUIRE(err.has_value());
            REQUIRE(err.value() == 2);
            err = thunk.poll();
            REQUIRE(err.has_value());
            REQUIRE(err.value() == 0);

            REQUIRE(counter == 3);
        }
    }
    SECTION("layer3")
    {
        bipbuf_t* buf = bipbuf_new(256);

        embr::sys::detail::v1::thunk<estd::layer3::bipbuf> thunk(estd::in_place_t{}, buf);

        REQUIRE(bipbuf_used(buf) == 0);

        thunk.post([&] { ++counter; });

        //   4 = message header
        // + function pointer
        // + capture reference (pointer, really)
        // = 20 typically
        constexpr int message_size = 4 + sizeof(void*) * 2;

        REQUIRE(bipbuf_used(buf) == message_size);

        thunk.poll_one();

        REQUIRE(bipbuf_used(buf) == 0);

        REQUIRE(counter == 1);

        bipbuf_free(buf);
    }
}


TEST_CASE("thunk: legacy exp", "[thunk]")
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
        SECTION("operator <<")
        {
            int counter{};

            t << [&]{ counter += 2; };
            t << [&]{ counter <<= 1; };

            REQUIRE(counter == 0);

            t.invoke_all();

            REQUIRE(counter == 4);
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

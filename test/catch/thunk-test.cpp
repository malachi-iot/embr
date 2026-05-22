#include <catch2/catch_all.hpp>

#include <future>
#include <random>
#include <vector>

#include <embr/exp/thunk.h>
#include <embr/internal/msg-bipbuf.h>

#include "mem/test-mem-data.h"

// Excellent breakdown of functor behavior:
// https://ricomariani.medium.com/std-function-teardown-and-discussion-a4f148929809

// DEBT: Instead of tracker/tracked use estd internal underpinnings for
// shared_ptr

// 05OCT25
// This has large overlap with "delegate_queue" and was coming along well to displace it IIRC
// IIRC I detoured to beef up estd functor behavior to better handle dtor operations here and never
// resumed

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

struct test_mutex
{
    std::timed_mutex mutex;

    bool lock()
    {
        return mutex.try_lock_for(std::chrono::milliseconds(200));
    }

    void unlock()
    {
        mutex.unlock();
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

#define BIPBUF_TEST_ENABLE1 1
#define BIPBUF_TEST_ENABLE2 1

TEST_CASE("msg_bipbuf", "[bipbuf]")
{
    SECTION("layer1")
    {
        using type = embr::internal::msg_bipbuf<estd::layer1::bipbuf<128>>;
        using message = type::message;

        type mbb;
        estd::errc err;

#if BIPBUF_TEST_ENABLE1
        SECTION("basic")
        {
            err = mbb.push([](message* m)
                {

                }, 10);

            REQUIRE(err == estd::errc{});
            // FIX: Since we are operating on pointers to payload and message,
            // we need to consider alignment.
            REQUIRE(mbb.buf().used() == sizeof(message) + 10);
        }
#endif
#if BIPBUF_TEST_ENABLE2
        SECTION("async")
        {
            std::vector<std::future<int>> futures;
            std::mt19937 gen{}; // fixed seed: deterministic sequence
            int sum1 = 0;
            int sum2 = 0;
            test_mutex mutex;

            // NOTE: Start too many and I think async goes to defer mode, which then pseudo-deadlocks
            // buffer availability.  Still sorting that out
            for(int i = 0; i < 10; ++i)
            {
                futures.push_back(std::async(std::launch::async, [&, i]
                    {
                        //CAPTURE(i);       // Catch2 will body slam you if you try this.  Don't CAPTURE
                                            // in a bunch of async threads
                        mutex.lock();
                        int v = gen();
                        mutex.unlock();
                        //err = mbb.emplace<int>(mutex, v);
                        int retries = 0;
                        for(;
                            retries < 10 && (err = mbb.emplace<int>(mutex, v)) == estd::errc::not_enough_memory;
                            ++retries)
                        {
                            //printf("\nLooping");
                            //VERIFY(retries < 10);
                            //if(retries > 10)
                                //FAIL("Too many failed allocation attempts");
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }
                        //printf("Queued err=%d retries=%d\n", err, retries);
                        //VERIFY(err == estd::errc{});
                        return v;
                    }));
            }

            for(std::future<int>& future : futures)
            {
                sum1 += future.get();
                mbb.pop([&](message* m)
                    {
                        int* v = (int*)m->payload();
                        sum2 += *v;
                        REQUIRE(m->sz == sizeof(int));
                    }, mutex);
            }

            REQUIRE(sum1 == sum2);
        }
#endif
    }
}

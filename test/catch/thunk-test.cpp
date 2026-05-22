#include <catch2/catch_all.hpp>

#include <future>
#include <queue>
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
            std::queue<int> parity;
            int sum1 = 0;
            int sum2 = 0;
            test_mutex mutex;

            // NOTE: Almost there, still fails sometimes
            for(int i = 0; i < 100; ++i)
            {
                futures.push_back(std::async(std::launch::async, [&, i]
                    {
                        //CAPTURE(i);       // Catch2 will body slam you if you try this.  Don't CAPTURE
                                            // in a bunch of async threads
                        mutex.mutex.lock();
                        int v = gen();
                        mutex.unlock();
                        //err = mbb.emplace<int>(mutex, v);
                        int retries = 0;
                        for(;
                            retries < 50 && (err = mbb.emplace<int>(mutex, v)) != estd::errc{};
                            ++retries)
                        {
                            //printf("\nLooping");
                            //VERIFY(retries < 10);
                            //if(retries > 10)
                                //FAIL("Too many failed allocation attempts");
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        }
                        // printf is currently better behaved in async than cerr
                        if(err != estd::errc{})
                            printf("Queued i=%d err=%d retries=%d\n", i, err, retries);
                        else
                        {
                            mutex.mutex.lock();
                            parity.push(v);
                            mutex.unlock();
                        }
                        //VERIFY(err == estd::errc{});
                        return v;
                    }));
            }

            int attempts = 0;
            int active;
            int i = 0;

            do
            {
                active = 0;
                for(std::future<int>& future : futures)
                {
                    if(future.valid() == false) continue;

                    ++active;

                    if(future.wait_for(std::chrono::milliseconds(5)) == std::future_status::timeout) continue;

                    sum1 += future.get();

                    int v;
                    mutex.mutex.lock();
                    int v_parity = parity.front();
                    int parity_size = parity.size();
                    parity.pop();
                    mutex.mutex.unlock();

                    err = mbb.pop([&](message* m)
                        {
                            int* v_ptr = (int*)m->payload();
                            v = *v_ptr;
                            sum2 += *v_ptr;
                            REQUIRE(m->sz == sizeof(int));
                        }, mutex);

                    REQUIRE(err == estd::errc{});

                    CAPTURE(parity_size, i);

                    REQUIRE(v == v_parity);

                    ++i;
                }

                ++attempts;
            }
            while(active);

            REQUIRE(sum1 == sum2);
        }
#endif
    }
}

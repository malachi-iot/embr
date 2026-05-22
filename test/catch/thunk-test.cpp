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

template <unsigned ms>
struct test_mutex
{
    std::timed_mutex mutex;

    bool lock()
    {
        return mutex.try_lock_for(std::chrono::milliseconds(ms));
    }

    void unlock()
    {
        mutex.unlock();
    }
};

TEST_CASE("thunk")
{
    embr::sys::detail::v1::thunk<estd::layer1::bipbuf<256>> t_new;
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

// Extra logging (non-error)
#define BIPBUF_TEST_ASYNC_LOG 0

// DEBT: Move all this to a dedicated bipbuf area, and some of this may even eventually
// scoot its way up to estd
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
        SECTION("list")
        {
            std::queue<int> parity, parity_gen;
            std::mt19937 gen{}; // fixed seed: deterministic sequence

            for(int i = 0; i < 100; ++i)
            {
                int v = gen() % 100;
                parity_gen.push(v);

                err = mbb.emplace<int>({}, v);

                if(err == estd::errc::not_enough_memory)
                {
                    // Dequeue a pseudo random amount
                    for(int dq_count = 1 + (v % 8); !mbb.empty() && dq_count > 0; --dq_count)
                    {
                        REQUIRE(!parity.empty());
                        int v_parity = parity.front();

                        err = mbb.pop([&](message* m)
                            {
                                auto payload = (int*)m->payload();

                                REQUIRE(*payload == v_parity);
                            });

                        REQUIRE(err == estd::errc{});

                        parity.pop();
                    }
                }
                else
                {
                    parity.push(v);
                }
            }
        }
#endif
#if BIPBUF_TEST_ENABLE2
        SECTION("async: fixed")
        {
            std::vector<std::future<int>> futures;
            std::mt19937 gen{}; // fixed seed: deterministic sequence

            std::vector<int> generated, parity;
            std::mutex parity_mutex;

            int sum1 = 0;
            int sum2 = 0;

            // Trying for the most aggressive 0ms timeout to test our spinwait/retry
            // Still never hit it though it's always OOM
            test_mutex<0> mutex;
            int retry_total = 0;

            for(int i = 0; i < 100; ++i)
            {
                futures.push_back(std::async(std::launch::async, [&, i]
                    {
                        //CAPTURE(i);       // Catch2 will body slam you if you try this.  Don't CAPTURE
                                            // in a bunch of async threads
                        const int v = gen() % 100;

                        // parity_mutex ensures that 'parity' vector stays lock-step with what we've
                        // pushed into mbb - regardless of any blockages along the way
                        parity_mutex.lock();
                        generated.push_back(v);

                        int retries = 0;
                        int err_retained = -1;
                        estd::errc err;

                        // Attempt to emplace generated value.  Might get rejected due to OOM or lock
                        for(;
                            retries < 50 && (err = mbb.emplace<int>(mutex, v)) != estd::errc{};
                            ++retries)
                        {
                            parity_mutex.unlock();

                            std::this_thread::sleep_for(std::chrono::milliseconds(20));
                            if(err != estd::errc{}) err_retained = int(err);

                            parity_mutex.lock();
                        }
                        // printf is currently better behaved in async than cerr
                        if(err != estd::errc{})
                            printf("Q FAIL i=%d err=%d retries=%d\n", i, err, retries);
                        else
                        {
#if BIPBUF_TEST_ASYNC_LOG
                            if(retries > 0)
                            {
                                printf("Q OK i=%d retained=%d retries=%d\n", i, err_retained, retries);
                            }
#endif

                            // NOTE: parity queue won't be 100% in generated order since we might get
                            // blocked on our emplace
                            parity.push_back(v);
                        }
                        parity_mutex.unlock();

                        retry_total += retries;

                        return v;
                    }));
            }

            int attempts = 0;
            int active;
            int parity_index = 0;

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
                    parity_mutex.lock();
                    int v_parity = parity[parity_index];
                    int parity_size = parity.size();
                    int generated_size = generated.size();
                    parity_mutex.unlock();

                    // Although generated values are probably aplenty, we may not have parity yet due to
                    // blocking enqueues.
                    if(parity_index >= parity_size) continue;

                    err = mbb.pop([&](message* m)
                        {
                            int* v_ptr = (int*)m->payload();
                            v = *v_ptr;
                            sum2 += *v_ptr;
                            REQUIRE(m->sz == sizeof(int));
                        }, mutex);

                    REQUIRE(err == estd::errc{});

                    CAPTURE(generated_size, parity_size, parity_index, retry_total);
                    //CAPTURE(generated);
                    CAPTURE(parity);

                    REQUIRE(v == v_parity);

                    ++parity_index;
                }

                ++attempts;

#if BIPBUF_TEST_ASYNC_LOG
                printf("Cycle: active=%d\n", active);
#endif
            }
            while(active);

            REQUIRE(sum1 == sum2);
        }
#endif
        SECTION("async: varied")
        {
            using message = decltype(mbb)::message;
            std::vector<std::future<void>> futures;
            std::mt19937 gen{}; // fixed seed: deterministic sequence
            test_mutex<0> mutex;

            for(int i = 0; i < 100; ++i)
            {
                mbb.push([&](message* m)
                {

                }, 10, mutex);
                auto f = [&, i]
                {
                };

                futures.push_back(std::async(std::launch::async, std::move(f)));
            }
        }
    }
}

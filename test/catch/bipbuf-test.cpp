#include <catch2/catch_all.hpp>

#include <future>
#include <queue>
#include <random>
#include <thread>
#include <vector>

#include <embr/internal/msg-bipbuf.h>

#define BIPBUF_TEST_ENABLE1 1
#define BIPBUF_TEST_ENABLE2 1
#define BIPBUF_TEST_ENABLE3 1

// Extra logging (non-error)
#define BIPBUF_TEST_ASYNC_LOG 0

template <class Buf>
using bipbuf = embr::internal::msg_bipbuf<Buf>;

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

template <class Buf>
static void test_list(bipbuf<Buf>& mbb, std::mt19937 gen)
{
    using message = typename bipbuf<Buf>::message;
    std::queue<int> parity, parity_gen;

    for(int i = 0; i < 100; ++i)
    {
        int v = gen() % 100;
        parity_gen.push(v);

        estd::errc err = mbb.template emplace<int>({}, v);

        if(err == estd::errc::not_enough_memory)
        {
            // Dequeue a pseudo random amount
            for(int dq_count = 1 + (v % 8); !mbb.empty() && dq_count > 0; --dq_count)
            {
                REQUIRE(!parity.empty());
                int v_parity = parity.front();

                err = mbb.pop({}, [&](message* m)
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

template <class Buf>
static void test_async_fixed(bipbuf<Buf>& mbb, std::mt19937 gen)
{
    using message = typename bipbuf<Buf>::message;
    std::vector<std::future<int>> futures;

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
                    retries < 50 && (err = mbb.template emplace<int>(mutex, v)) != estd::errc{};
                    ++retries)
                {
                    parity_mutex.unlock();

                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                    if(err != estd::errc{}) err_retained = int(err);

                    parity_mutex.lock();
                }
                // printf is currently better behaved in async than cerr
                if(err != estd::errc{})
                    printf("Q FAIL i=%d err=%d retries=%d\n", i, int(err), retries);
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

            estd::errc err = mbb.pop(mutex, [&](message* m)
                {
                    int* v_ptr = (int*)m->payload();
                    v = *v_ptr;
                    sum2 += *v_ptr;
                    REQUIRE(m->sz == sizeof(int));
                });

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

template <class Buf>
static void test_async_varied(bipbuf<Buf>& mbb, std::mt19937 gen)
{
    using namespace std::chrono_literals;
    using message = typename bipbuf<Buf>::message;
    std::vector<std::future<void>> futures;
    test_mutex<0> mutex;

    // FIX: Alignment concerns are still present

    for(int i = 0; i < 50; ++i)
    {
        unsigned sz = gen() % 32;

        auto f = [&, sz, i]
        {
            int tries = 0;
            estd::errc err;
            do
            {
                err = mbb.push(mutex, [sz](message* m)
                    {
                        std::memset(m->payload(), '0' + sz, sz);
                    }, sz);

                if(err == estd::errc::not_enough_memory)
                    std::this_thread::sleep_for(20ms);

            }   while(err != estd::errc{} && ++tries < 100);

            if(tries > 1)
            {
                if(err != estd::errc{})
                {
                    printf("async varied Q FAIL: i=%d sz=%d err=%d\n", i, sz, int(err));
                }
                else
                {
#if BIPBUF_TEST_ASYNC_LOG
                    printf("async varied Q OK: i=%d tries=%d\n", i, tries);
#endif
                }
            }
        };

        futures.push_back(std::async(std::launch::async, std::move(f)));
    }

    int active = 0;

    do
    {
        active = 0;
        for(std::future<void>& future : futures)
        {
            if(future.valid() == false) continue;

            ++active;

            if(future.wait_for(10ms) == std::future_status::timeout) continue;

            future.get();

            estd::errc err = mbb.pop(mutex, [&](message* m)
                {
#if BIPBUF_TEST_ASYNC_LOG
                    printf("async varied: sz=%d\n", m->sz);
#endif
                    auto payload = (const char*)m->payload();
                    int sz = m->sz;
                    REQUIRE(sz <= 32);
                    REQUIRE(std::all_of(payload, payload + sz, [sz](char c)
                        {
                            return c == '0' + sz;
                        }));
                });

            REQUIRE(err == estd::errc{});
        }

#if BIPBUF_TEST_ASYNC_LOG
        printf("async varied: active=%d\n", active);
#endif

    }   while(active);
}


TEST_CASE("bipartite buffer: message-oriented", "[msg-bipbuf]")
{
    SECTION("layer1")
    {
        bipbuf<estd::layer1::bipbuf<128>> mbb;
        std::mt19937 gen{}; // NOLINT fixed seed: deterministic sequence = what we want
        using message = decltype(mbb)::message;

        SECTION("basic")
        {
            estd::errc err = mbb.push({}, [](message* m)
                {

                }, 10);

            REQUIRE(err == estd::errc{});
            // FIX: Since we are operating on pointers to payload and message,
            // we need to consider alignment.
            REQUIRE(mbb.buf().used() == sizeof(message) + 10);
        }
        SECTION("list")
        {
            test_list(mbb, gen);
        }
        SECTION("async: fixed")
        {
            test_async_fixed(mbb, gen);
        }
        SECTION("async: varied")
        {
            test_async_varied(mbb, gen);
        }
    }
}
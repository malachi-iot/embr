#include <queue>
#include <random>

#include <estd/chrono.h>
#include <estd/port/freertos/mutex.h>
#include <estd/port/freertos/thread.h>

#include <embr/internal/msg-bipbuf.h>

#include "unit-test.h"

using namespace embr;

namespace rtos = estd::freertos::wrapper;

template <class Buf>
using bipbuf = internal::msg_bipbuf<Buf>;

// I hate copy/pasting.  However abstracting this test to run in both Catch2
// and unity would be silly.
// See also PGESP#105 
// https://bitbucket.org/malachib/playground.esp/issues/105/catch2
template <class Buf>
static void test_list(bipbuf<Buf>& mbb, std::mt19937& gen)
{
    using message = typename bipbuf<Buf>::message;

    // DEBT: For synthetic testing a queue like this is OK.  However we really
    // ought to use an estd fixed allocation flavor
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
                TEST_ASSERT_FALSE(parity.empty());
                int v_parity = parity.front();

                err = mbb.pop({}, [&](message* m)
                    {
                        auto payload = (int*)m->payload();

                        TEST_ASSERT_TRUE(*payload == v_parity);
                    });

                TEST_ASSERT_TRUE(err == estd::errc{});

                parity.pop();
            }
        }
        else
        {
            parity.push(v);
        }
    }
}

using namespace estd::chrono_literals;

struct test_mutex
{
    estd::freertos::timed_mutex<true> mutex_;

    bool lock()
    {
        return mutex_.try_lock_for(50ms);
    }

    void unlock() { mutex_.unlock(); }
};

template <class Bipbuf>
struct shared_type
{
    using message = typename Bipbuf::message;

    rtos::task parent;
    Bipbuf& mbb;
    std::mt19937& gen;
    test_mutex mutex;

    constexpr static int loops = 10;

    void push()
    {
        const uint8_t sz = gen() % 32;

        [[maybe_unused]]
        estd::errc err = mbb.push(mutex, [sz](message* m)
        {
            estd::fill_n((char*)m->payload(), sz, sz);
        }, sz);
    }

    void do_things()
    {
        for(int i = 0; i < loops; ++i) push();

        parent.notify_give(0);
    }
};

using layer1_type = bipbuf<estd::layer1::bipbuf<128>>;

// TODO: Not used yet, this is theoretically a way to overcome part fault
// on unit test failure.  In particular, worker tasks are still running but
// shared state goes out of scope.  I don't want to allocate it globally,
// so the idea is if we allocate it on the stack before RUN_TEST occurs,
// that might do the trick.
[[maybe_unused]]
static union
{
    shared_type<layer1_type>* layer1;
}   shared;

static constexpr int task_count = 3;

static void wait_for_worker_finish()
{
    for(int i = 0; i < task_count; ++i)
        ulTaskNotifyTakeIndexed(0, pdFALSE, portMAX_DELAY);
}


template <class Bipbuf, class Buf>
static void test_async(shared_type<Bipbuf>& shared, bipbuf<Buf>& mbb, std::mt19937& gen)
{
    using message = typename bipbuf<Buf>::message;

    auto f = [](void* arg)
    {
        // DO NOT LOG HERE! 2K stack isn't enough for that
        auto shared = (shared_type<Bipbuf>*) arg;

        shared->do_things();

        vTaskDelete(nullptr);
    };

    rtos::task tasks[task_count];
    for(rtos::task& task : tasks)
    {
        BaseType_t r = task.create(f, "bipbuf worker", 2048, &shared, 1);
        TEST_ASSERT_TRUE(r);
    }

    int counter = 0;

    for(int i = 0; i < task_count * shared.loops && counter < 1000;++counter)
    {
        estd::errc err = mbb.pop(shared.mutex, [&](const message* p)
        {
            char temp[32];
            uint8_t sz = p->payload_size();
            estd::fill_n(temp, sz, sz);
            TEST_ASSERT_EQUAL_HEX8_ARRAY(temp, p->payload(), sz);
        });

        TEST_ASSERT_NOT_EQUAL(estd::errc::no_lock_available, err);

        if(err == estd::errc{})
        {
            ++i;
        }
    }

    wait_for_worker_finish();

    TEST_ASSERT_LESS_THAN(10000, counter);
}

// Although std::async and pthreads are an option, it feels like a better test to
// do with real FreeRTOS tasks directly

static void test_layer1()
{
    // DEBT: Whoa!  mt19937 uses over 2k of stack!!  Find an alternative
    static std::mt19937 gen{}; // NOLINT fixed seed: deterministic sequence = what we want
    using type = layer1_type;
    {
        type mbb;
        test_list(mbb, gen);
    }
    {
        type mbb;
        shared_type<layer1_type>
        // DEBT: Why does out mutex buddy need this explicit initializer?  Odd.
        shared{rtos::task::current(), mbb, gen, {}};
        ::shared.layer1 = &shared;
        
        test_async(*::shared.layer1, mbb, gen);
    }
}

#ifdef ESP_IDF_TESTING
TEST_CASE("bipbuf", "[bipbuf]")
#else
void test_bipbuf()
#endif
{
    {
        RUN_TEST(test_layer1);
    }
}
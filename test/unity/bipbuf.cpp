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

template <class Buf>
static void test_async(bipbuf<Buf>& mbb, std::mt19937& gen)
{
    using message = typename bipbuf<Buf>::message;

    struct shared_type
    {
        rtos::task parent;
        bipbuf<Buf>& mbb;
        std::mt19937& gen;
        test_mutex mutex;

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
            for(int i = 0; i < 10; ++i) push();

            parent.notify_give(0);
        }

        // DEBT: Why does out mutex buddy need this explicit initializer?  Odd.
    }   shared{rtos::task::current(), mbb, gen, {}};

    auto f = [](void* arg)
    {
        // DO NOT LOG HERE! 2K stack isn't enough for that
        auto shared = (shared_type*) arg;

        shared->do_things();

        vTaskDelete(nullptr);
    };

    rtos::task tasks[2];
    for(rtos::task& task : tasks)
    {
        BaseType_t r = task.create(f, "bipbuf worker", 2048, &shared, 1);
        TEST_ASSERT_TRUE(r);
    }

    int counter = 0;

    for(int i = 0; i < 20 && counter < 1000;++counter)
    {
        estd::errc err = mbb.pop(shared.mutex, [&](message* p)
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

    // Wait for workers to finish
    for(auto& _ : tasks)
        ulTaskNotifyTakeIndexed(0, pdFALSE, portMAX_DELAY);

    TEST_ASSERT_LESS_THAN(10000, counter);
}

// Although std::async and pthreads are an option, it feels like a better test to
// do with real FreeRTOS tasks directly

static void test_layer1()
{
    // DEBT: Whoa!  mt19937 uses over 2k of stack!!  Find an alternative
    static std::mt19937 gen{}; // NOLINT fixed seed: deterministic sequence = what we want
    using type = bipbuf<estd::layer1::bipbuf<128>>;
    
    {
        type mbb;
        test_list(mbb, gen);
    }
    {
        type mbb;
        test_async(mbb, gen);
    }
}

#ifdef ESP_IDF_TESTING
TEST_CASE("bipbuf", "[bipbuf]")
#else
void test_bipbuf()
#endif
{
    RUN_TEST(test_layer1);
}
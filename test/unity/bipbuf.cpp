#include <queue>
#include <random>

#include <esp_log.h>

#include <estd/chrono.h>
#include <estd/port/freertos/mutex.h>
#include <estd/port/freertos/semaphore.h>
#include <estd/port/freertos/thread.h>

#include <embr/platform/freertos/mutex.h>

#include <embr/internal/msg-bipbuf.h>

#include "shared.h"
#include "unit-test.h"

using namespace embr;

static const char* TAG = "embr::unity::bipbuf";

template <class Buf>
using bipbuf = internal::msg_bipbuf<Buf>;

static constexpr int task_count = 3;

// I hate copy/pasting.  However abstracting this test to run in both Catch2
// and unity would be silly.  EDIT: Changed my mind.  This particular test
// could be abstracted with an is_true functor
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

namespace rtos = estd::freertos::wrapper;

using namespace estd::chrono_literals;

template <class Bipbuf,
    ESTD_CPP_CONCEPT(embr::internal::concepts::Mutex) Mutex,
    class F, class OnRetry>
estd::errc push_with_retry(Bipbuf& mbb, Mutex&& mutex, F&& f,
    unsigned sz, int retry_max, OnRetry&& on_retry)
{
    for(int retry = 0; retry < retry_max;)
    {
        const estd::errc err = mbb.push(
            std::forward<Mutex>(mutex),
            std::forward<F>(f),
            sz);

        if(err == estd::errc{}) return err;

        ++retry;

        if(retry >= retry_max)  return err;

        on_retry();
    }

    abort();
}

// DEBT: Put this definition elsewhere
test::shared::semaphore test::shared::finished;

template <class Bipbuf,
    ESTD_CPP_CONCEPT(embr::internal::concepts::Mutex) Mutex = embr::freertos::timed_mutex<50>>
struct shared_type : test::shared
{
    using message = typename Bipbuf::message;

    Bipbuf& mbb;
    std::mt19937& gen;
    // DEBT: Why does out mutex buddy need this explicit initializer?  Odd.
    Mutex mutex{};

    constexpr static int loops = 10;
    constexpr static int sample = 32;

    shared_type(Bipbuf& bb, std::mt19937& gen) : mbb{bb}, gen{gen}  {}

    void push()
    {
        // NOTE: We want 0-size once in a while just for fuller coverage
        const uint8_t sz = gen() % sample;
        auto f = [sz](message* m)
        {
            estd::fill_n((char*)m->payload(), sz, sz);
        };

        [[maybe_unused]]
        estd::errc err;// = mbb.push(mutex, f, sz);

        // DEBT: Do some retries and maybe some metrics gathering - test will eventually
        // fail without the retries portion

        err = push_with_retry(mbb, mutex, f, sz, 5, []
            {
                vTaskDelay(5);
            });

        TEST_ASSERT_EQUAL(estd::errc{}, err);
    }

    void do_things()
    {
        for(int i = 0; i < loops; ++i) push();

        finish();
    }
};

using layer1_type = bipbuf<estd::layer1::bipbuf<128>>;
using layer3_type = bipbuf<estd::layer3::bipbuf>;

// TODO: Not used yet, this is theoretically a way to overcome part fault
// on unit test failure.  In particular, worker tasks are still running but
// shared state goes out of scope.  I don't want to allocate it globally,
// so the idea is if we allocate it on the stack before RUN_TEST occurs,
// that might do the trick.
[[maybe_unused]]
static union
{
    shared_type<layer1_type>* layer1;
    shared_type<layer3_type>* layer3;
}   shared;



template <class Bipbuf, class Mutex, class Buf>
static void test_async(shared_type<Bipbuf, Mutex>& shared, bipbuf<Buf>& mbb)
{
    using message = typename bipbuf<Buf>::message;

    auto f = [](void* arg)
    {
        // DO NOT LOG HERE! 2K stack isn't enough for that
        auto shared = (shared_type<Bipbuf, Mutex>*) arg;

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
    int i = 0;
    int bytes_processed = 0;

    for(; i < task_count * shared.loops && counter < 1000; ++counter)
    {
        estd::errc err = mbb.pop(shared.mutex, [&](const message* p)
        {
            char temp[32];
            uint8_t sz = p->payload_size();
            if(sz == 0) return;
            estd::fill_n(temp, sz, sz);
            TEST_ASSERT_NOT_NULL(p->payload());
            TEST_ASSERT_EQUAL_HEX8_ARRAY(temp, p->payload(), sz);
            bytes_processed += sz;
        });

        TEST_ASSERT_NOT_EQUAL(estd::errc::no_lock_available, err);

        if(err == estd::errc{})
        {
            ++i;
        }
        else
        {
            vTaskDelay(1);
        }
    }

    ESP_LOGD(TAG, "bytes_processed=%d", bytes_processed);

    shared.wait(task_count);

    TEST_ASSERT_EQUAL(i, task_count * shared.loops);
    TEST_ASSERT_LESS_THAN(1000, counter);

    // If our random distribution is OK, we should always exceed 25% of maximum
    // random byte processed possibility
    TEST_ASSERT_GREATER_THAN(shared.loops * task_count * (shared.sample / 4),
        bytes_processed);
}

// Although std::async and pthreads are an option, it feels like a better test to
// do with real FreeRTOS tasks directly

// DEBT: Whoa!  mt19937 uses over 2k of stack!!  Find an alternative
static std::mt19937 gen{}; // NOLINT fixed seed: deterministic sequence = what we want

static void test_layer1()
{
    using type = layer1_type;
    {
        type mbb;
        test_list(mbb, gen);
    }
#if ESTD_OS_FREERTOS
    {
        type mbb;
        shared_type<layer1_type>
            shared{mbb, gen};
        ::shared.layer1 = &shared;
        
        test_async(*::shared.layer1, mbb);
    }
    {
        type mbb;
        shared_type<layer1_type, embr::freertos::hw_mutex>
            shared{mbb, gen};

        test_async(shared, mbb);
    }
#endif
}

static void test_layer3()
{
#if ESTD_OS_FREERTOS
    union
    {
        bipbuf_t bipbuf;
        char storage[sizeof(bipbuf) + 128];
    };

    {
        bipbuf_init(&bipbuf, 128);

        // TODO: Reconsider needing in_place_t here, since init varies very little
        layer3_type mbb(estd::in_place_t{}, &bipbuf);
        static shared_type<layer3_type>
            shared{mbb, gen};

        test_async(shared, mbb);
    }
    {
        bipbuf_init(&bipbuf, 128);

        // TODO: Reconsider needing in_place_t here, since init varies very little
        layer3_type mbb(estd::in_place_t{}, &bipbuf);
        static shared_type<layer3_type, embr::freertos::hw_mutex>
            shared{mbb, gen};

        test_async(shared, mbb);
    }
#endif
}


#ifdef ESP_IDF_TESTING
TEST_CASE("bipbuf", "[bipbuf]")
#else
void test_bipbuf()
#endif
{
#if !USE_TASK_NOTIFICATION_SHARED
    test::shared::finished.create_counting(3, 0);
#endif

    {
        RUN_TEST(test_layer1);
    }
    {
        RUN_TEST(test_layer3);
    }
}
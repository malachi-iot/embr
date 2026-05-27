#include "unit-test.h"

#include <esp_log.h>

#include <estd/chrono.h>

#if ESTD_OS_FREERTOS

#include <estd/port/freertos/mutex.h>
#include <estd/port/freertos/thread.h>

#include <embr/platform/freertos/mutex.h>
#include <embr/thunk.h>

#include "shared.h"

using namespace embr;
namespace rtos = estd::freertos::wrapper;

static const char* TAG = "embr::unity::thunk";

// NOTE: Be careful.  'post' operations are expected to be very fast,
// but exotic captures might slow things down.
using hw_mutex = embr::freertos::hw_mutex;
using timed_mutex = embr::freertos::timed_mutex<50>;

// DEBT: dedup with bipbuf code
static constexpr int task_count = 3;

static void wait_for_worker_finish()
{
    for(int i = 0; i < task_count; ++i)
    {
        [[maybe_unused]]
        unsigned v = ulTaskNotifyTakeIndexed(0, pdFALSE, portMAX_DELAY);

        //ESP_LOGI(TAG, "wait_for_worker_finish: %u", v);
    }

    ESP_LOGV(TAG, "wait_for_worker_finish: done");
}

template <class Thunk>
struct shared_type
{
    rtos::task parent = rtos::task::current();
    int counter = 0;
    Thunk thunk;
    constexpr static int loops = 10;

    void post()
    {
        for(int i = 0; i < loops; ++i)
        {
            estd::errc err;

            for(int retries = 0;
                retries < 10 &&
                (err = thunk.post([&] { ++counter; })) != estd::errc{};
                ++retries)
            {
                vTaskDelay(5);
            }
        }
    }

    void do_things()
    {
        post();

        //puts("thunk give");
        parent.notify_give(0);
    }
};



static void test_thunk_ll()
{
    int counter = 0;
    sys::v1::layer1::thunk<256, hw_mutex> thunk;

    thunk.post([&] { ++counter; });
    thunk.poll_one();

    TEST_ASSERT_EQUAL(1, counter);
}

// TODO: Pull in gcc stack warnings from estd

template <class Mutex>
using shared_layer1 = shared_type<sys::v1::layer1::thunk<256, Mutex>>;

template <class Mutex>
using shared_layer3 = shared_type<sys::v1::layer3::thunk<Mutex>>;

template <class Shared>
static void test_thunk_async_ll(Shared& shared)
{
    rtos::task tasks[task_count];
    for(rtos::task& task : tasks)
    {
        // DO NOT IN WORKER! 2K stack isn't enough for that
        BaseType_t r = task.create(
            embr::test::shared::worker<Shared>, "thunk worker", 2048, &shared, 1);
        TEST_ASSERT_TRUE(r);
    }

    int counter = 0;

    for(int i = 0; i < task_count * shared.loops && counter < 1000; ++counter)
    {
        estd::errc err = shared.thunk.poll_one();

        TEST_ASSERT_NOT_EQUAL(estd::errc::no_lock_available, err);

        if(err == estd::errc{})
        {
            ++i;
        }
        else
        {
            vTaskDelay(1);
        }
    
        /*
        for(int retries = 0;
            retries < 10 &&
            (err = shared.thunk.poll_one()) != estd::errc{};
            ++retries)
        {
            //vTaskDelay(5);
        }   */
    }

    wait_for_worker_finish();

    TEST_ASSERT_EQUAL(task_count * shared.loops, shared.counter);
    TEST_ASSERT_LESS_THAN(1000, counter);
}

void test_thunk_async()
{
    {
        shared_layer1<timed_mutex> shared;
        test_thunk_async_ll(shared);
    }
    {
        shared_layer1<hw_mutex> shared;
        test_thunk_async_ll(shared);
    }
    {
        //bipbuf_t bb;
        //shared_layer3<timed_mutex> shared(estd::in_place_t{}, &bb);
    }
}

#ifdef ESP_IDF_TESTING
TEST_CASE("thunk", "[thunk]")
#else
void test_thunk()
#endif
{
    // FIX: Because scheduler or bipbuf tests leaves task notification in a naughty state,
    // we have to consume one extra here
    [[maybe_unused]]
    unsigned v = ulTaskNotifyTakeIndexed(0, pdFALSE, portMAX_DELAY);

    RUN_TEST(test_thunk_ll);
    RUN_TEST(test_thunk_async);
}

#endif

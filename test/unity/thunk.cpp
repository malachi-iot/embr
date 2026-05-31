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

template <class Thunk>
struct shared_type : test::shared
{
    int counter = 0;
    Thunk thunk;
    constexpr static int loops = 50;
    int total_retries = 0;

    shared_type() = default;

    template <class ...Args>
    constexpr shared_type(Args&&...args) : thunk(std::forward<Args>(args)...) {}

    void post()
    {
        for(int i = 0; i < loops; ++i)
        {
            estd::errc err;

            err = thunk.post_with_retry(
                [&]{ ++counter; }, 10,
                [&]{ ++total_retries; vTaskDelay(1); });

            TEST_ASSERT_EQUAL(estd::errc{}, err);
        }
    }

    void do_things()
    {
        post();

        //puts("thunk give");
        finish();
    }
};



static void test_thunk_ll()
{
    int counter = 0;
    sys::v1::layer1::thunk<256, hw_mutex> thunk;
    estd::errc err;

    //using namespace embr::freertos;

    thunk.post([&] { ++counter; });
    thunk << [&] {++counter; };
    thunk.poll_one();

    TEST_ASSERT_EQUAL(1, counter);

    thunk.poll_one();

    TEST_ASSERT_EQUAL(2, counter);

    err = thunk.poll_one();

    TEST_ASSERT_EQUAL(estd::errc::no_message_available, err);
    TEST_ASSERT_EQUAL(2, counter);
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
        // DO NOT LOG IN WORKER! 2K stack isn't enough for that
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

    test::shared::wait(task_count);

    ESP_LOGD(TAG, "total retries: %d", shared.total_retries);

    TEST_ASSERT_EQUAL(task_count * shared.loops, shared.counter);
    TEST_ASSERT_LESS_THAN(1000, counter);
    TEST_ASSERT_GREATER_THAN(0, shared.total_retries);
}

void test_thunk_async()
{
    {
        ESP_LOGD(TAG, "async layer1 timed_mutex");
        shared_layer1<timed_mutex> shared;
        test_thunk_async_ll(shared);
    }
    {
        ESP_LOGD(TAG, "async layer1 hw_mutex");
        shared_layer1<hw_mutex> shared;
        test_thunk_async_ll(shared);
    }
    {
        union
        {
            bipbuf_t bb;
            char backing[sizeof(bb) + 256];
        };
        ESP_LOGD(TAG, "async layer3 hw_mutex");
        shared_layer3<timed_mutex> shared(estd::in_place_t{}, &bb, 256);
        test_thunk_async_ll(shared);
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

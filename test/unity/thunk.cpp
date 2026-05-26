#include "unit-test.h"

#include <estd/chrono.h>
#include <estd/port/freertos/mutex.h>
#include <estd/port/freertos/thread.h>

#include <embr/platform/freertos/mutex.h>
#include <embr/thunk.h>

#if ESTD_OS_FREERTOS

using namespace embr;
namespace rtos = estd::freertos::wrapper;

// NOTE: Be careful.  'post' operations are expected to be very fast,
// but exotic captures might slow things down.
using hw_mutex = embr::freertos::hw_mutex;

// DEBT: dedup with bipbuf code
static constexpr int task_count = 3;

static void wait_for_worker_finish()
{
    for(int i = 0; i < task_count; ++i)
        ulTaskNotifyTakeIndexed(0, pdFALSE, portMAX_DELAY);
}

template <class Thunk>
struct shared_type
{
    Thunk thunk;

    void do_things()
    {

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

static void test_thunk_async()
{
    int counter = 0;
    using type = shared_type<sys::v1::layer1::thunk<256, hw_mutex>>;
    type shared;

    auto f = [](void* arg)
    {
        // DO NOT LOG HERE! 2K stack isn't enough for that
        auto shared = (type*) arg;

        shared->do_things();

        vTaskDelete(nullptr);
    };
  
    rtos::task tasks[task_count];
    for(rtos::task& task : tasks)
    {
        BaseType_t r = task.create(f, "thunk worker", 2048, &shared, 1);
        TEST_ASSERT_TRUE(r);
    }

    wait_for_worker_finish();
}

#ifdef ESP_IDF_TESTING
TEST_CASE("thunk", "[thunk]")
#else
void test_thunk()
#endif
{
    RUN_TEST(test_thunk_ll);
    //RUN_TEST(test_thunk_async);   // No notify give yet
}

#endif

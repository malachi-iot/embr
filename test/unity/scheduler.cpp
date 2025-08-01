#include <embr/scheduler.h>

#include "unit-test.h"

#include "../catch/scheduler-test.h"

#if ESTD_OS_FREERTOS
#include <embr/platform/freertos/scheduler.h>

using namespace embr::scheduler::freertos;
using namespace test::scheduler;
using namespace estd::chrono_literals;
using clock_type = estd::chrono::freertos_clock;
using time_point = typename clock_type::time_point;
using item_type = ref_nexter<time_point, typename clock_type::duration>;

static void test_scheduler_with_notify()
{
    const time_point now = clock_type::now();

    v1::layer1::scheduler_with_notify<item_type, 10> scheduler;
    item_type i1(1, 40ms), i2(2, 40ms);

    scheduler.reschedule(&i1);
    TEST_ASSERT_FALSE(scheduler.process_one(0ms));
    i1.next(now + 50ms);
    // priorty_queue had a glitch prohibiting this before 0.8.9
#if ESTD_VERSION > ESTD_BUILD_SEMVER(0, 8, 8)
    scheduler.reschedule(&i1);
    TEST_ASSERT_TRUE(scheduler.process_one(0ms));
#else
#error estd >= 0.8.9 expected, but not found
#endif
}

static void test_scheduler_with_event()
{
    const time_point now = clock_type::now();

    v1::layer1::scheduler_with_event<item_type, 10> scheduler;
    item_type i1(1, 40ms), i2(2, 40ms);

    i1.next(now + 50ms);
}

#endif

#ifdef ESP_IDF_TESTING
TEST_CASE("scheduler tests", "[scheduler]")
#else
void test_scheduler()
#endif
{
#if ESTD_OS_FREERTOS
    test_scheduler_with_event();
    test_scheduler_with_notify();
#endif
}
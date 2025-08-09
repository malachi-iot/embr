#include <estd/chrono.h>

#include <embr/scheduler.h>

#include "unit-test.h"

#include "../catch/scheduler-test.h"

using namespace test::scheduler;
using namespace estd::chrono_literals;

using process_result = embr::scheduler::v1::detail::scheduler_base::process_result;

#if ESTD_OS_FREERTOS
#include <embr/platform/freertos/scheduler.h>

namespace freertos {

using namespace embr::scheduler::freertos;
using clock_type = estd::chrono::freertos_clock;
using time_point = typename clock_type::time_point;
using item_type = ref_nexter<time_point, typename clock_type::duration>;

static void test_scheduler_with_notify()
{
    const time_point now = clock_type::now();

    v1::layer1::scheduler_with_notify<item_type, 10> scheduler;
    item_type i1(1, 40ms), i2(2, 40ms);

    scheduler.reschedule(&i1);
    TEST_ASSERT_EQUAL(process_result::PROCESSED_AND_RESCHEDULED, scheduler.process_one(0ms));
    i1.next(now + 50ms);
    // priorty_queue had a glitch prohibiting this before 0.8.9
#if ESTD_VERSION > ESTD_BUILD_SEMVER(0, 8, 8)
    scheduler.reschedule(&i1);
    TEST_ASSERT_EQUAL(process_result::UNPROCESSED, scheduler.process_one(0ms));
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
    scheduler.reschedule(&i1);
}

}

#endif

#ifdef ESP_IDF_TESTING

#include <embr/platform/esp-idf/v1/scheduler.hpp>


static void test_gptimer_scheduler()
{
    //using clock = estd::chrono::esp_clock;
    using item_type = ref_nexter<uint64_t>;
    embr::scheduler::esp_idf::v1::gptimer_scheduler<
        embr::scheduler::item_traits<item_type>,
        estd::layer1::vector<item_type*, 10>> s;
    process_result r1;

#if INCLUDE_vTaskPrioritySet
    const UBaseType_t prio = uxTaskPriorityGet(nullptr);
    vTaskPrioritySet(nullptr, prio + 2);
#endif

    constexpr uint64_t increment = 40000;

    item_type i1(1, increment), i2(2, increment);

    i1.next(increment);
    i2.next(increment);
    TEST_ASSERT_EQUAL(0, i1.last());

    TEST_ASSERT_EQUAL(ESP_OK, s.init());

    s.reschedule(&i1);
    s.reschedule(&i2);

    uint64_t marker = esp_timer_get_time();
    TEST_ASSERT_EQUAL(ESP_OK, s.start());

#if ENABLED1
    uint64_t counter = 0;

    // FIX: Somehow doing this correlates with gptimer_context::alarm_cb crashing
    ESP_ERROR_CHECK(s.timer().get_raw_count(&counter));

    TEST_ASSERT_UINT_WITHIN(10, 500, counter);
#endif

    r1 = s.process_one(100ms);

    TEST_ASSERT_EQUAL(process_result::PROCESSED_AND_RESCHEDULED, r1);
    TEST_ASSERT_EQUAL(increment * 2, i1.next());
    TEST_ASSERT_EQUAL(increment, i2.next());
    TEST_ASSERT_UINT_WITHIN(10, increment, i1.last());

    marker = esp_timer_get_time();

    r1 = s.process_one(100ms);

    TEST_ASSERT_EQUAL(process_result::PROCESSED_AND_RESCHEDULED, r1);

    TEST_ASSERT_EQUAL(increment * 2, i2.next());

    ESP_ERROR_CHECK(s.stop());

#if INCLUDE_vTaskPrioritySet
    vTaskPrioritySet(nullptr, prio);
#endif

    s.deinit();
}

#endif

#ifdef ESP_IDF_TESTING
TEST_CASE("scheduler tests", "[scheduler]")
#else
void test_scheduler()
#endif
{
#if ESTD_OS_FREERTOS
    freertos::test_scheduler_with_event();
    freertos::test_scheduler_with_notify();
#endif
#ifdef ESP_IDF_TESTING
    test_gptimer_scheduler();
#endif
}
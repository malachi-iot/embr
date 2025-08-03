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
using process_result = embr::scheduler::v1::detail::scheduler_base::process_result;

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
}

#endif

#ifdef ESP_IDF_TESTING

#include <embr/platform/esp-idf/v1/scheduler.hpp>


static void test_gptimer_scheduler()
{
    using item_type = ref_nexter<uint64_t>;
    embr::scheduler::esp_idf::v1::gptimer_scheduler<
        embr::scheduler::item_traits<item_type>,
        estd::layer1::vector<item_type*, 10>> s;

    s.init();
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
    test_scheduler_with_event();
    test_scheduler_with_notify();
#endif
#ifdef ESP_IDF_TESTING
    test_gptimer_scheduler();
#endif
}
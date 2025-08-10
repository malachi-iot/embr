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

#include <esp_log.h>

#include <embr/platform/esp-idf/v1/scheduler.hpp>

const char* TAG = "unity::scheduler";

namespace idf {

// DEBT: Making these static because if unit test fails, 's' doesn't go through
// its regular deinitialization yet ISR keeps running looking for 's',
// ending up with a crash.  Better way to do this is wrap with a ctor/dtor, but
// keeping this global will at least inhibit crashes, though ISR will continue to run
using item_type = ref_nexter<uint64_t>;
using gptimer_clock = embr::scheduler::esp_idf::v1::detail::gptimer_clock;

static embr::scheduler::esp_idf::v1::gptimer_scheduler<
    embr::scheduler::item_traits<item_type>,
    estd::layer1::vector<item_type*, 10>> s;

// Different chips, USB loggings and QEMU or not affect timings.
// Especially QEMU which appears to have an incomplete gptimer implementation (resets
// counter more often than we'd like)

// Couldn't find any ways of runtime detecting QEMU, so making a compile time flag here
#define QEMU 0
#define PHASE1_TIMING_TEST  1

static void test_gptimer_scheduler()
{
    //using clock = estd::chrono::esp_clock;
    process_result r1;
    BaseType_t notification_received = false;

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

    uint64_t marker = esp_timer_get_time();
    r1 = s.process_one(0ms);
    const uint64_t p1_delta = esp_timer_get_time() - marker;
    TEST_ASSERT_EQUAL(process_result::UNPROCESSED, r1);

    s.reschedule(&i1);
    s.reschedule(&i2);

    ESP_LOGD(TAG, "test_gptimer_scheduler: phase 1");

    TEST_ASSERT_EQUAL(ESP_OK, s.start());

    uint64_t counter = 0;

    ESP_ERROR_CHECK(s.timer().get_raw_count(&counter));

    // Tested with ESP32C6 Xiao
#if CONFIG_IDF_TARGET_ARCH_RISCV
    TEST_ASSERT_UINT_WITHIN(18, 20, counter);
#elif QEMU
    // QEMU goes pretty slow here
    TEST_ASSERT_UINT_WITHIN(250, 300, counter);
#elif CONFIG_IDF_TARGET_ESP32S3
    TEST_ASSERT_UINT_WITHIN(20, 25, counter);
#endif

    r1 = s.process_one(100ms, &notification_received);

    marker = esp_timer_get_time();

    TEST_ASSERT_TRUE(notification_received);

#if PHASE1_TIMING_TEST
    ESP_ERROR_CHECK(s.timer().get_raw_count(&counter));

    TEST_ASSERT_EQUAL(0, i2.last());

    ESP_LOGD(TAG, "test_gptimer_scheduler: phase 2 counter=%" PRIu64 ", clock.now()=%" PRIu64
        ", i1.last()=%" PRIu64,
        counter, s.clock().raw_now(), i1.last());

#if CONFIG_IDF_TARGET_ARCH_RISCV
    TEST_ASSERT_UINT_WITHIN(100, increment, counter);
#elif QEMU
    // Clocks in at ~150000us despite receiving notification above
    TEST_ASSERT_UINT_WITHIN(500, increment, counter);
#elif CONFIG_IDF_TARGET_ESP32S3
    TEST_ASSERT_UINT_WITHIN(200, increment, counter);
#endif

    TEST_ASSERT_EQUAL(process_result::PROCESSED_AND_RESCHEDULED, r1);
    TEST_ASSERT_EQUAL(increment * 2, i1.next());
    TEST_ASSERT_EQUAL(increment, i2.next());
#if CONFIG_IDF_TARGET_ARCH_RISCV
    TEST_ASSERT_UINT_WITHIN(20, increment, i1.last());
#elif QEMU
    // Clocks in at > ~200000us despite above ~150000us.  Something badly wrong here,
    // last() is supposed to be a fixed point in time, even with slow-ass QEMU.  ChatGPT
    // indicates that QEMU falsely restarts counter & that's why
    TEST_ASSERT_UINT_WITHIN(250, increment, i1.last());
#elif CONFIG_IDF_TARGET_ESP32S3
    TEST_ASSERT_UINT_WITHIN(50, increment, i1.last());
#endif

#endif  // PHASE1_TIMING_TEST

    uint64_t marker2 = esp_timer_get_time();
    uint64_t delta = p1_delta + (marker2 - marker);

    r1 = s.process_one(100ms);

    ESP_LOGD(TAG, "test_gptimer_scheduler: phase 3 delta=%" PRIu64
        ", clock.now()=%" PRIu64 ", i2.last()=%" PRIu64,
        delta, s.clock().raw_now(), i2.last());

    TEST_ASSERT_EQUAL(process_result::PROCESSED_AND_RESCHEDULED, r1);

#if CONFIG_IDF_TARGET_ARCH_RISCV || CONFIG_IDF_TARGET_ESP32S3
    TEST_ASSERT_UINT_WITHIN(delta, increment, i2.last());
#endif
    TEST_ASSERT_EQUAL(increment * 2, i2.next());

    ESP_ERROR_CHECK(s.stop());

#if INCLUDE_vTaskPrioritySet
    vTaskPrioritySet(nullptr, prio);
#endif

    s.deinit();

    ESP_LOGD(TAG, "test_gptimer_scheduler: phase 4");
}

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
    idf::test_gptimer_scheduler();
#endif
}
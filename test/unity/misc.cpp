#include <estd/chrono.h>
#include <embr/profiler.h>

#include "unit-test.h"

using namespace embr;

static void test_profiler()
{
#if ESP_PLATFORM
    using clock = estd::chrono::esp_clock;
#elif ARDUINO
    using clock = estd::chrono::arduino_clock;
#else
    // DEBT: Use high resolution clock here
    using clock = estd::chrono::system_clock;
#endif

    Profiler<clock> p;

    uint64_t elapsed = p.mark().count();

    TEST_ASSERT_GREATER_THAN(0, elapsed);
}


static void test_profiler_raw()
{
#if ESP_PLATFORM
    using clock = embr::esp_idf::chrono::timer;

    Profiler<clock> p;

    volatile uint64_t elapsed = p.mark();
    elapsed = p.mark();

    TEST_ASSERT_GREATER_THAN(0, elapsed);
#endif
}

#ifdef ESP_IDF_TESTING
TEST_CASE("catchall tests", "[misc]")
#else
void test_misc()
#endif
{
    RUN_TEST(test_profiler);
    RUN_TEST(test_profiler_raw);
}

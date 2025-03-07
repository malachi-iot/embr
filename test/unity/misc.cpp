#include <estd/chrono.h>
#include <embr/profiler.h>

#include "unit-test.h"

using namespace embr;

static void test_profiler()
{
    using clock = estd::chrono::esp_clock;

    Profiler<clock> p;

    uint64_t elapsed = p.mark().count();

    TEST_ASSERT_GREATER_THAN(0, elapsed);

    // TODO: test embr::esp_idf::chrono::esp_timer variety
}

#ifdef ESP_IDF_TESTING
TEST_CASE("catchall tests", "[misc]")
#else
void test_misc()
#endif
{
    RUN_TEST(test_profiler);
}

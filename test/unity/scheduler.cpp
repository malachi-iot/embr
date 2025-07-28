#include <embr/scheduler.h>

#include "unit-test.h"

#include "../catch/scheduler-test.h"

#if ESTD_OS_FREERTOS
#include <embr/platform/freertos/scheduler.h>

using namespace embr::scheduler::freertos;
using namespace test::scheduler;

static void test_scheduler_with_notify()
{
    v1::layer1::scheduler_with_notify<ref_nexter<int>, 10> scheduler;
}
#endif

#ifdef ESP_IDF_TESTING
TEST_CASE("scheduler tests", "[scheduler]")
#else
void test_scheduler()
#endif
{
#if ESTD_OS_FREERTOS
    test_scheduler_with_notify();
#endif
}
#include <embr/scheduler.h>
#include <embr/platform/freertos/scheduler.h>

#include "unit-test.h"

#ifdef ESP_IDF_TESTING
TEST_CASE("scheduler tests", "[scheduler]")
#else
void test_scheduler()
#endif
{

}
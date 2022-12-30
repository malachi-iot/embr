#include "unity.h"

#include <embr/platform/esp-idf/timer.h>
#include <embr/platform/esp-idf/gptimer.h>

using namespace embr::esp_idf;

TEST_CASE("general purpose timer v4", "[gptimer v4]")
{
    v4::Timer timer(TIMER_GROUP_0, TIMER_0);
}

TEST_CASE("general purpose timer v4", "[gptimer v4]")
{
    v5::Timer timer;
}

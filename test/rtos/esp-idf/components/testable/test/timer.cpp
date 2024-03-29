#include "unity.h"

#include <embr/platform/esp-idf/fwd.h>

using namespace embr::esp_idf;

#if FEATURE_EMBR_ESP_TIMER_SCHEDULER
#include <embr/platform/esp-idf/timer.h>

TEST_CASE("general purpose timer v4", "[gptimer v4]")
{
    v4::Timer timer(TIMER_GROUP_0, TIMER_0);
}
#endif

#if ESTD_IDF_VER >= ESTD_IDF_VER_5_0_0
#include <embr/platform/esp-idf/gptimer.h>

TEST_CASE("general purpose timer v5", "[gptimer v5]")
{
    v5::Timer timer;
}
#endif
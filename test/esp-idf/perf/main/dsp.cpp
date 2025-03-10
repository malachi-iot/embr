#include <esp_log.h>

#include <embr/dsp/precalc.h>

#include "perf.h"

static const char* TAG = "embr::perf::dsp";

void test_dsp()
{
    Profiler p;

    embr::dsp::init_sin_table();

    float j = 0;

    p.reset();

    for(float v = 0; v < 10; v += 0.001)
    {
        j += std::sin(v);
    }

    duration m1 = p.mark();

    ESP_LOGI(TAG, "std::sin %f %" PRIu64 "us", j, m1.count());

    j = 0;

    p.reset();

    for(float v = 0; v < 10; v += 0.001)
    {
        j += embr::dsp::sin_lookup(v);
    }

    duration m2 = p.mark();

    ESP_LOGI(TAG, "embr::dsp::sin_lookup %f %" PRIu64 "us", j, m2.count());
}
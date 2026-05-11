#include <esp_log.h>

#include <embr/dsp/precalc.h>

#include "perf.h"

static const char* TAG = "embr::perf::dsp";

void test_dsp()
{
    constexpr double max = 6, incr = 0.001;
    constexpr unsigned sz = EMBR_DSP_PRECALC_TABLE_SZ;

    Profiler p;

    embr::dsp::init_sin_table();
    auto sin_table = (float*)heap_caps_malloc(sz * sizeof(float), MALLOC_CAP_INTERNAL);
    embr::dsp::init_sin_table(estd::span<float, sz>(sin_table));

    float j = 0;

    p.reset();

    for(float v = 0; v < max; v += incr)
    {
        j += std::sin(v);
    }

    duration m1 = p.mark();

    ESP_LOGI(TAG, "std::sin %f m1=%" PRIu64 "us", j, m1.count());

    j = 0;

    p.reset();

    //using pc = embr::dsp::detail::precalc<embr::dsp::PRECALC_DEFAULT>;

    for(float v = 0; v < max; v += incr)
    {
        j += embr::dsp::sin_lookup(v);
        //j += pc::sin_ll<sz>(embr::dsp::detail::sin_table, v);
        // Somehow this guy (direct pointer) is always fastest
        //j += pc::sin_ll<sz>(sin_table, v);
    }

    duration m2 = p.mark();

    ESP_LOGI(TAG, "embr::dsp::sin_lookup %f m2=%" PRIu64 "us", j, m2.count());
}
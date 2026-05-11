#include <esp_log.h>

#include <embr/dsp/precalc.h>
#include <embr/dsp/v1/drc.h>
#include <embr/dsp/v1/fp.h>

#include "perf.h"

static const char* TAG = "embr::perf::dsp";

using namespace embr;

static void test_sin()
{
    constexpr double max = 6, incr = 0.001;
    constexpr unsigned sz = EMBR_DSP_PRECALC_TABLE_SZ;

    perf::Profiler p;

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

template <class Scalar>
static void test_drc(const char* tag)
{
    Scalar out[16] {};

    using drc_type = dsp::drc<Scalar>;

    constexpr const Scalar in_buf1[] { 0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6 };
    int count = 1000;

    drc_type drc;
    constexpr typename drc_type::params params
    {
        0.7, 0.8, 0.01
    };

    perf::Profiler p;

    while(count-- > 0)
    {
        process(drc, params, in_buf1, in_buf1 + std::size(in_buf1), out);
    }

    duration m1 = p.mark();

    ESP_LOGI(TAG, "embr::dsp::drc %s m1=%" PRIu64 "us", tag, m1.count());
}

void test_dsp()
{
    using fp4_12 = dsp::v1::fixed_point<4, 12,
        dsp::v1::fixed_point_options::FP_SIGNED |
        dsp::v1::fixed_point_options::FP_IMPLICIT>;
    using fp8_24 = dsp::v1::fixed_point<8, 24,
        dsp::v1::fixed_point_options::FP_SIGNED |
        dsp::v1::fixed_point_options::FP_IMPLICIT>;

    test_sin();
    test_drc<float>("float");
    test_drc<double>("double");
    test_drc<fp4_12>("fp4.12");
    test_drc<fp8_24>("fp8.24");
}

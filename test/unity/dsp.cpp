#include <embr/profiler.h>
#include <embr/dsp/precalc.h>

#include "unit-test.h"

using namespace embr;

template <dsp::v1::precalc_modes mode, std::size_t sz, typename T, class T2>
static void compare(const estd::span<T, sz>& table, T2 v)
{
    const T v1 = dsp::v1::sin_lookup<mode>(table, v);
    const T v1_ = estd::sin(v);

    // DEBT: Crude promotion of T to most precise.  May not work for things
    // like fixed point
    TEST_ASSERT_LESS_THAN_DOUBLE(0.001, std::abs(std::abs(v1) - std::abs(v1_)));
}

template <dsp::v1::precalc_modes mode, std::size_t sz, typename T>
static void test_sin_table(const T* table)
{
    estd::span<const T, sz> t(table);

    compare<mode>(t, 0.2);
    compare<mode>(t, 0.6);
    compare<mode>(t, 1.0);
    compare<mode>(t, 3.0);
}

static void test_sinf()
{
#if FEATURE_EMBR_DSP_PRECALC_TABLE
    static constexpr std::size_t sz = EMBR_DSP_PRECALC_TABLE_SZ;
    test_sin_table<dsp::v1::PRECALC_HALF, sz>(dsp::detail::v1::sin_table);
#else
#endif
}

static void test_sind()
{

}

#ifdef ESP_IDF_TESTING
TEST_CASE("dsp tests", "[dsp]")
#else
void test_dsp()
#endif
{
#if FEATURE_EMBR_DSP_PRECALC_TABLE
    dsp::init_sin_table();
#endif

    RUN_TEST(test_sinf);
    RUN_TEST(test_sind);
}

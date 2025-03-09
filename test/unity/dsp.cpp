#include <embr/profiler.h>
#include <embr/dsp/precalc.h>

#include "unit-test.h"

using namespace embr;

template <dsp::v1::precalc_modes mode, std::size_t sz, typename T, class T2>
static void compare(const estd::span<T, sz>& table, T2 v)
{
    auto v_ = static_cast<estd::remove_const_t<T>>(v);
    T v1 = dsp::v1::sin_lookup<mode>(table, v_);
    T v1_ = std::sin(v_);

    //CAPTURE(v1, v1_, v);
    TEST_ASSERT_LESS_THAN(0.001, std::abs(std::abs(v1) - std::abs(v1_)));
}

template <dsp::v1::precalc_modes mode, std::size_t sz, typename T>
static void test_sin_table(const T* table)
{
    //estd::span<const T, sz> t(table);

    //compare<mode>(t, 0.2);
}

static void test_sinf()
{
    static constexpr std::size_t sz = EMBR_DSP_PRECALC_TABLE_SZ;
    test_sin_table<dsp::v1::PRECALC_HALF, sz>(dsp::v1::detail::sin_table);
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
}

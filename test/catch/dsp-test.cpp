#include <catch2/catch_all.hpp>

#include <embr/dsp/precalc.h>

using namespace embr;

template <dsp::v1::precalc_modes mode = dsp::v1::PRECALC_HALF, class T, std::size_t N>
void compare(const estd::span<T, N>& t, T v)
{
    T v1 = dsp::v1::sin_lookup<mode>(t, v);
    T v1_ = sin(v);

    CAPTURE(v1, v1_, v);
    REQUIRE(std::abs(std::abs(v1) - std::abs(v1_)) < 0.001);
}

TEST_CASE("dsp")
{
    double* table = new double[8192];
    auto table2 = new float[4096];
    estd::span<double, 8192> t(table);
    estd::span<float, 4096> t2(table2);

    dsp::v1::init_sin_table(t);
    dsp::v1::init_sin_table(t2);

    compare(t, 0.2);
    compare(t, 1.0);
    compare(t, 3.0);
    compare(t, 9.0);
    compare(t, 32.0);

    compare(t2, 0.2f);

    dsp::v1::init_sin_table<dsp::PRECALC_QUART>(t2);

    compare<dsp::PRECALC_QUART>(t2, 0.2f);
    compare<dsp::PRECALC_QUART>(t2, 3.0f);
    compare<dsp::PRECALC_QUART>(t2, 6.0f);
    compare<dsp::PRECALC_QUART>(t2, 9.0f);
    compare<dsp::PRECALC_QUART>(t2, 32.0f);
}

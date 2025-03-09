#pragma once

#include <cmath>

#include <estd/span.h>

#if ESP_PLATFORM
// FIX: ESP-IDF seems to treat 'y' here as a 1, but regular GCC doesn't.  This is too wide
// a descrepency, I am not understanding something
#define EMBR_DSP_PRECALC_TABLE CONFIG_EMBR_DSP_PRECALC_TABLE
#else
#define EMBR_DSP_PRECALC_TABLE 1
#endif

#if CONFIG_EMBR_DSP_PRECALC_TABLE_SZ
#define EMBR_DSP_PRECALC_TABLE_SZ   CONFIG_EMBR_DSP_PRECALC_TABLE_SZ
#elif !EMBR_DSP_PRECALC_TABLE_SZ
#define EMBR_DSP_PRECALC_TABLE_SZ   4096
#endif

namespace embr { namespace dsp { inline namespace v1 {

enum precalc_modes
{
    PRECALC_FULL,
    PRECALC_HALF,
    PRECALC_QUART,

    PRECALC_DEFAULT = PRECALC_HALF
};

namespace detail {

extern float sin_table[EMBR_DSP_PRECALC_TABLE_SZ];

template <precalc_modes>
struct precalc;

template <>
struct precalc<PRECALC_HALF>
{
    template <typename T, estd::size_t N>
    void static init_sin(estd::span<T, N> table)
    {
        T* data = table.data();
        for(int i = 0; i < N; i++, data++)
        {
            const auto v = i * M_PI / N;
            *data = std::sin(v);
        }
    }

    template <typename T, estd::size_t N>
    constexpr static T sin(const estd::span<T, N>& table, T v)
    {
        constexpr unsigned mask = N * 2 - 1;

        unsigned i = std::round(v * N / M_PI);

        i &= mask;

        if(i < N)
            return table[i];
        else
            return -table[i - N];
    }
};


template <>
struct precalc<PRECALC_QUART>
{
    template <typename T, estd::size_t N>
    void static init_sin(estd::span<T, N> table)
    {
        T* data = table.data();
        for(int i = 0; i < N; i++, data++)
        {
            const auto v = i * M_PI / (2.0 * N);
            *data = std::sin(v);
        }
    }

    template <typename T, estd::size_t N>
    constexpr static T sin(const estd::span<T, N>& table, T v)
    {
        constexpr unsigned mask = N * 4 - 1;

        int i = std::round(v * 2 * N / M_PI);

        i &= mask;

        if(i < N)
            return table[i];
        else if(i < N * 2)
            //return table[1 - (i - N) + N];
            return table[1 - i + N * 2];
        else if(i < N * 3)
            return -table[i - N * 2];
        else
            //return -table[N - 1 - (i - N * 3)];
            return -table[1 - i + N * 4];
    }
};

}

template <precalc_modes mode = PRECALC_DEFAULT, typename T, estd::size_t N>
void init_sin_table(estd::span<T, N> table)
{
    detail::precalc<mode>::init_sin(table);
}

template <precalc_modes mode = PRECALC_DEFAULT, typename T, estd::size_t N>
constexpr T sin_lookup(const estd::span<T, N>& table, T v)
{
    return detail::precalc<mode>::sin(table, v);
}

constexpr float sin_lookup(float v)
{
    return detail::precalc<PRECALC_DEFAULT>::sin(estd::span<float, EMBR_DSP_PRECALC_TABLE_SZ>{detail::sin_table}, v);
}

void init_sin_table();

}}}

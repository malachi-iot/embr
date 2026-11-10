#pragma once

// TODO: Once https://github.com/malachi-iot/estdlib/issues/141 comes online,
// refactor std::sin, etc to estd::sin

#include <assert.h>

#include <estd/cmath.h>
#include <estd/span.h>

#include "config.h"
#include "enum.h"
#include "fwd.h"

// FIX: Just for Arduino.  estd v0.8.10 proper does this by way of macro push/pop
#undef round

namespace embr { namespace dsp {

namespace internal {

// DEBT: Strongly consider forcing this to float every time.  However, precision loss
// may hurt us and we'll occasionally round the wrong way
template <class T>
constexpr unsigned fast_round(T v)
{
#if EMBR_DSP_FAST_ROUND_STRICT
    assert(v >= 0);
#endif

    return static_cast<unsigned>(v + 0.5f);
}

}

namespace detail { inline namespace v1 {

// UNTESTED
template <>
struct precalc<PRECALC_FULL>
{
    template <typename T, estd::size_t N>
    void static init_sin(estd::span<T, N> table)
    {
        T* data = table.data();
        for(int i = 0; i < N; i++, ++data)
        {
            const auto v = i * 2 * M_PI / N;
            *data = estd::sin(v);
        }
    }

    template <bool do_mask = true, typename T, estd::size_t N, typename T2>
    constexpr static T sin(const estd::span<T, N>& table, T2 v)
    {
        constexpr unsigned mask = N - 1;
        constexpr T2 multiplier = N / (2 * M_PI);

#if EMBR_DSP_FAST_ROUND
        unsigned i = internal::fast_round(v * multiplier);
#else
        unsigned i = estd::round(v * multiplier);
#endif

        if(do_mask)     i &= mask;

        return table[i];
    }
};

template <>
struct precalc<PRECALC_HALF>
{
    template <typename T, estd::size_t N>
    void static init_sin(estd::span<T, N> table)
    {
        T* data = table.data();
        for(unsigned i = 0; i < N; i++, ++data)
        {
            const auto v = i * M_PI / N;
            *data = estd::sin(v);
        }
    }

    template <estd::size_t N, bool do_mask = true, typename T, typename T2>
    EMBR_DSP_INLINE constexpr static T sin_ll(const T* table, T2 v)
    {
        constexpr unsigned mask = N * 2 - 1;
        constexpr T2 N_div_pi = N / M_PI;
#if EMBR_DSP_FAST_ROUND
        unsigned i = internal::fast_round(v * N_div_pi);
#else
        auto i = static_cast<unsigned>(estd::round(v * N_div_pi));
#endif

        ESTD_CPP_IF_CONSTEXPR(do_mask)   i &= mask;

        return i < N ? table[i] : -table[i - N];
    }

    template <typename T, estd::size_t N, typename T2>
    EMBR_DSP_INLINE constexpr static T sin(const estd::span<T, N>& table, T2 v)
    {
        return sin_ll<N>(table.data(), v);
    }
};


template <>
struct precalc<PRECALC_QUART>
{
    template <typename T, estd::size_t N>
    void static init_sin(estd::span<T, N> table)
    {
        T* data = table.data();
        for(unsigned i = 0; i < N; i++, ++data)
        {
            const auto v = i * M_PI / (2.0 * N);
            *data = estd::sin(v);
        }
    }

    template <typename T, estd::size_t N, typename T2>
    constexpr static T sin(const estd::span<T, N>& table, T2 v)
    {
        constexpr unsigned mask = N * 4 - 1;

        int i = estd::round(v * 2 * N / M_PI);

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


}}

}}

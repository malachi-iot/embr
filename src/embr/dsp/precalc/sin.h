#pragma once

// TODO: Once https://github.com/malachi-iot/estdlib/issues/141 comes online,
// refactor std::sin, etc to estd::sin

#include <estd/cmath.h>
#include <estd/span.h>

#include "config.h"
#include "enum.h"
#include "fwd.h"

namespace embr { namespace dsp {

namespace detail { inline namespace v1 {

// UNTESTED
template <>
struct precalc<PRECALC_FULL>
{
    template <typename T, estd::size_t N>
    void static init_sin(estd::span<T, N> table)
    {
        T* data = table.data();
        for(int i = 0; i < N; i++, data++)
        {
            const auto v = i * 2 * M_PI / N;
            *data = std::sin(v);
        }
    }

    template <bool do_mask = true, typename T, estd::size_t N, typename T2>
    constexpr static T sin(const estd::span<T, N>& table, T2 v)
    {
        constexpr unsigned mask = N - 1;
        constexpr T2 multiplier = N / (2 * M_PI);

        unsigned i = std::round(v * multiplier);

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
        for(int i = 0; i < N; i++, data++)
        {
            const auto v = i * M_PI / N;
            *data = std::sin(v);
        }
    }

    template <estd::size_t N, bool do_mask = true, typename T, typename T2>
    constexpr static T sin_ll(const T* table, T2 v)
    {
        constexpr unsigned mask = N * 2 - 1;
        constexpr T N_div_pi = N / M_PI;
        unsigned i = (unsigned)std::round(v * N_div_pi);

        if constexpr(do_mask)   i &= mask;

        return i < N ? table[i] : -table[i - N];
    }

    template <typename T, estd::size_t N, typename T2>
    constexpr static T sin(const estd::span<T, N>& table, T2 v)
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
        for(int i = 0; i < N; i++, data++)
        {
            const auto v = i * M_PI / (2.0 * N);
            *data = std::sin(v);
        }
    }

    template <typename T, estd::size_t N, typename T2>
    constexpr static T sin(const estd::span<T, N>& table, T2 v)
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


}}

}}

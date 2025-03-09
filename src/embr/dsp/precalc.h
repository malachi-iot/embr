#pragma once

#include <cmath>

#include <estd/span.h>

namespace embr { namespace dsp { inline namespace v1 {

template <typename T, estd::size_t N>
void init_sin_table(estd::span<T, N> table)
{
    T* data = table.data();
    for(int i = 0; i < N; i++, data++)
    {
        double v = i * M_PI / N;
        *data = sin(v);
    }
}

template <typename T, estd::size_t N>
constexpr T sin_lookup(const estd::span<T, N>& table, T v)
{
    constexpr unsigned mask = N * 2 - 1;

    unsigned i = std::round(v * N / M_PI);

    i &= mask;

    if(i < N)
        return table[i];
    else
        return -table[i - N];
}

void init_sin_table();

}}}

#pragma once

#include <estd/span.h>

#include "precalc/config.h"
#include "precalc/cos.h"
#include "precalc/sin.h"
#include "precalc/fwd.h"

namespace embr { namespace dsp { inline namespace v1 {

template <precalc_modes mode = PRECALC_DEFAULT, typename T, estd::size_t N>
void init_sin_table(estd::span<T, N> table)
{
    detail::precalc<mode>::init_sin(table);
}

template <precalc_modes mode = PRECALC_DEFAULT, typename T, estd::size_t N, typename T2>
constexpr T sin_lookup(const estd::span<T, N>& table, T2 v)
{
    return detail::precalc<mode>::sin(table, v);
}

constexpr float sin_lookup(float v)
{
    //return detail::sin_table[(int)(v * 3.14f)];
    return detail::precalc<PRECALC_DEFAULT>::sin(estd::span<const float, EMBR_DSP_PRECALC_TABLE_SZ>{detail::sin_table}, v);
}

void init_sin_table();

}}}

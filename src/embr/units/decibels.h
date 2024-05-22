#pragma once

#include "base.h"

// DEBT: Account for dB suffixes, such as:
// A - acoustic
// V - volt
// m - milliwatt
// https://www.softdb.com/blog/difference-between-db-dba/

namespace embr { namespace units {

namespace internal {

struct bels_tag {};

}

template <typename Rep, class Period = estd::ratio<1>, typename F = internal::passthrough<Rep> >
using bels = detail::unit<Rep, Period, internal::bels_tag, F>;

template <typename Rep, typename F = internal::passthrough<Rep> >
using decibels = bels<Rep, estd::deci, F>;


}}


namespace estd { namespace internal { namespace units {

template <>
struct traits<embr::units::internal::bels_tag>
{
    static constexpr const char* name() { return "bels"; }
    static constexpr const char* abbrev() { return "B"; }
};


}}}

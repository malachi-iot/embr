#pragma once

#include "base.h"

namespace embr { namespace units {

namespace internal {

struct volts_tag {};

}

///
/// @tparam Rep core unit size of underlying count/ticks
/// @tparam Period scaling ratio
/// @tparam F final conversion.  defaults to passhtrough (noop)
template <typename Rep, class Period = estd::ratio<1>, typename F = internal::passthrough<Rep> >
using volts = detail::unit<Rep, Period, internal::volts_tag, F>;

template <class Rep, typename F = internal::passthrough<Rep> >
using kilovolts = volts<Rep, estd::kilo, F>;

template <class Rep, typename F = internal::passthrough<Rep> >
using decivolts = volts<Rep, estd::deci, F>;

template <class Rep, typename F = internal::passthrough<Rep> >
using millivolts = volts<Rep, estd::milli, F>;

template <class Rep, typename F = internal::passthrough<Rep> >
using microvolts = volts<Rep, estd::micro, F>;


}}

namespace estd { namespace internal { namespace units {

template <>
struct traits<embr::units::internal::volts_tag>
{
    static constexpr const char* name() { return "volts"; }

    static constexpr const char* abbrev() { return "V"; }

    //static constexpr si::quantities quantity = si::quantities::electric_current;
};

}}}

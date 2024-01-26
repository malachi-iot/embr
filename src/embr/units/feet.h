#pragma once

#include "base.h"
#include "seconds.h"

namespace embr { namespace units {

namespace internal {

struct feet_tag {};

using foot_second_tag = compound_tag<feet_tag, seconds_tag>;

}

template <typename Rep, class Period = estd::ratio<1>, typename F = internal::passthrough<Rep> >
using feet = detail::unit<Rep, Period, internal::feet_tag, F>;

template <typename Rep, typename F = internal::passthrough<Rep> >
using inches = feet<Rep, estd::ratio<1, 12>, F>;

template <typename Rep, typename F = internal::passthrough<Rep> >
using yards = feet<Rep, estd::ratio<3>, F>;

template <typename Rep, typename F = internal::passthrough<Rep> >
using miles = feet<Rep, estd::ratio<5280>, F>;



}}
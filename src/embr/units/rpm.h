#pragma once

#include "base.h"

namespace embr { namespace units {

namespace internal { struct rpm_tag {}; }

template <class Rep, class Period = estd::ratio<1>, typename F = internal::passthrough<Rep> >
using rpm = detail::unit<Rep, Period, internal::rpm_tag, F>;

}}

namespace estd { namespace internal { namespace units {

template <>
struct traits<embr::units::internal::rpm_tag>
{
    static constexpr const char* name() { return "revolutions per minute"; }
    static constexpr const char* abbrev() { return "rpm"; }
};


}}}

#pragma once

// Measurement of fluid, not loudness.  See decibels for that

#include "base.h"

namespace embr { namespace units {

namespace internal {

struct liters_tag {};

struct ounces_tag {};

struct fluid_ounces_tag {};

}

template <typename Rep, class Period = estd::ratio<1>, typename F = internal::passthrough<Rep> >
using liters = detail::unit<Rep, Period, internal::liters_tag, F>;

template <typename Rep, class Period = estd::ratio<1>, typename F = internal::passthrough<Rep> >
using ounces = detail::unit<Rep, Period, internal::ounces_tag, F>;

template <typename Rep, class Period = estd::ratio<1>, typename F = internal::passthrough<Rep> >
using fluid_ounces = detail::unit<Rep, Period, internal::fluid_ounces_tag, F>;

template <typename Rep, typename F = internal::passthrough<Rep> >
using quarts = fluid_ounces<Rep, estd::ratio<32, 1>>;

template <typename Rep, typename F = internal::passthrough<Rep> >
using gallons = fluid_ounces<Rep, estd::ratio<128, 1>>;

}}


namespace estd { namespace internal { namespace units {

template <>
struct traits<embr::units::internal::liters_tag>
{
    static constexpr const char* name() { return "liters"; }

    static constexpr const char* abbrev() { return "L"; }

    static constexpr si::quantities quantity = si::quantities::amount_of_substance;
};


template <>
struct traits<embr::units::internal::ounces_tag>
{
    static constexpr const char* name() { return "ounces"; }

    static constexpr const char* abbrev() { return "oz"; }

    static constexpr si::quantities quantity = si::quantities::amount_of_substance;
};


template <>
struct traits<embr::units::internal::fluid_ounces_tag>
{
    static constexpr const char* name() { return "fluid ounces"; }

    static constexpr const char* abbrev() { return "fl. oz"; }

    static constexpr si::quantities quantity = si::quantities::amount_of_substance;
};

}}}

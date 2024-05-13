#pragma once

// Measurement of fluid, not loudness.  See decibels for that

#include "base.h"

namespace embr { namespace units {

namespace internal {

struct liters_tag {};

struct ounces_tag {};

struct fluid_ounces_tag {};

}

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

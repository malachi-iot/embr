#pragma once

#include "base.h"

namespace embr { namespace units {

namespace internal {

struct decibels_tag {};

}

}}


namespace estd { namespace internal { namespace units {

template <>
struct traits<embr::units::internal::decibels_tag>
{
    static constexpr const char* name() { return "decibels"; }

    static constexpr const char* abbrev() { return "dB"; }

    static constexpr si::quantities quantity = si::quantities::electric_current;
};


}}}

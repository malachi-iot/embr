#pragma once

#include "base.h"

namespace embr { namespace units {

namespace internal { struct kelvins_tag {}; }


}}

namespace estd::internal::units {

template <>
struct traits<embr::units::internal::kelvins_tag>
{
    static constexpr const char* name() { return "kelvins"; }

    static constexpr si::quantities quanitiy = si::quantities::temperature;
};


}
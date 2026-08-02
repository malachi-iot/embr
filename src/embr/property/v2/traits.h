#pragma once

#include <estd/policy/rfc.h>
#include <estd/type_traits.h>

#include "fwd.h"

namespace embr { inline namespace property { inline namespace v2 {

using rfc2119 = estd::internal::rfc::rfc2119;

namespace detail {

template <class T>
struct traits : estd::type_identity<T>
{
    static constexpr bool is_specialized = true;
    static constexpr rfc2119 has_from = rfc2119::should;
};

}

template <const char*>
struct traits
{
    static constexpr bool is_specialized = false;
};


}}}

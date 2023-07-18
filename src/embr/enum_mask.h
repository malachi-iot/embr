#pragma once

#include <estd/internal/platform.h>

#include <estd/type_traits.h>

#ifdef __cpp_variadic_templates
namespace embr {

namespace internal {


/// Compile-time ORs a list together
/// @tparam TCore Typecast from T - optional, one may specify same type for this and T
/// @tparam T root integer to OR against
/// @tparam vs list of Ts to OR together
template <typename TCore, typename T, T... vs>
struct or_all;

template <typename TCore, typename T, T v1>
struct or_all<TCore, T, v1> :
    estd::integral_constant<TCore, (TCore)v1>
{
};

template <typename TCore, typename T, T v1, T... vs>
struct or_all<TCore, T, v1, vs...> :
    estd::integral_constant<TCore, (TCore)v1 | or_all<TCore, T, vs...>::value>
{
};

template <class TEnum, TEnum... vs>
using enum_or_all = or_all<
    typename estd::underlying_type<TEnum>::type, TEnum, vs...>;

template <class TEnum, TEnum v>
struct enum_mask : estd::integral_constant<TEnum, v>
{
    typedef estd::integral_constant<TEnum, v> base_type;
    typedef typename base_type::value_type e;
    typedef typename base_type::value_type enum_type;
    typedef typename estd::underlying_type<enum_type>::type underlying_type;

    template <enum_type... vs>
    using or_all = embr::internal::or_all<underlying_type, enum_type, vs...>;

    // Determines if any of the specified enum_types are present by applying a bitmask
    // of all inquired types
    // NOTE: This returns underlying_type (the mask result), not bool
    template <enum_type ...vs>
    using mask = estd::integral_constant<underlying_type,
        (underlying_type)v & or_all<vs...>::value>;

    template <enum_type ...vs>
    constexpr static bool all()
    {
        return mask<vs...>::value == or_all<vs...>::value;
    }

    template <enum_type ...vs>
    constexpr static bool any()
    {
        return mask<vs...>::value;
    }
};

}

}
#else
#include "c++03/enum_mask.h"
#endif

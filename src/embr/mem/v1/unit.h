#pragma once

#include <estd/ratio.h>
#include <estd/units.h>

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <typename Rep, typename Period>
struct bytes_unit_traits : estd::units::detail::traits<Rep, Period, estd::internal::units::bytes_tag>
{
    static constexpr auto options = estd::units::detail::options::value_initialized;

    constexpr static Rep default_value() { return estd::numeric_limits<Rep>::max(); }
};

// TODO: Do unit_traits for human-readable descriptions, presuming existing bytes_traits doesn't already

}}

template <typename Rep, typename Period = estd::ratio<1>>
using bytes_unit = estd::units::v1::detail::unit<detail::v1::bytes_unit_traits<Rep, Period>>;


}}


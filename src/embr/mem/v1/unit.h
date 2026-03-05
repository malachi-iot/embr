#pragma once

#include <estd/ratio.h>
#include <estd/units.h>

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <typename Rep, typename Period>
struct bytes_unit_traits : estd::units::detail::traits<Rep, Period, estd::units::bytes_tag>
{
    static constexpr auto options =
        estd::units::detail::options::value_initialized |
        estd::units::detail::options::permissive;

    constexpr static Rep default_value() { return estd::numeric_limits<Rep>::max(); }
};

// TODO: Do si::traits in estd for human-readable descriptions, with kb, gb, etc. if not already
// existing

}}

template <typename Rep, typename Period = estd::ratio<1>>
using bytes_unit = estd::units::v1::detail::unit<detail::v1::bytes_unit_traits<Rep, Period>>;


}}


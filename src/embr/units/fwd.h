#pragma once

#include <estd/internal/units/fwd.h>

namespace embr { namespace units {

namespace internal {

template <class Rep>
using passthrough = estd::internal::units::passthrough<Rep>;

template <class T1, class T2>
using compound_tag = estd::internal::units::compound_tag<T1, T2>;

}

namespace detail {

template <typename Rep, class Period, class Tag, typename F>
using unit = estd::internal::units::unit_base<Rep, Period, Tag, F>;

}

}}

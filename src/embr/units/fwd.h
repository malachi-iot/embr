#pragma once

#include <estd/internal/units/fwd.h>

namespace embr { namespace units {

namespace internal {

template <class Rep>
using passthrough = estd::internal::units::passthrough<Rep>;

}

namespace detail {

template <typename Rep, class Period, class Tag, typename F>
using unit = estd::internal::units::unit_base<Rep, Period, Tag, F>;

}

}}

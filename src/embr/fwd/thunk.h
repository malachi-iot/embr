#pragma once

#include "../internal/mutex.h"
#include "../internal/fwd.h"

namespace embr { inline namespace sys {

namespace detail { inline namespace v1 {

// EXPERIMENTAL, unused
template <ESTD_CPP_CONCEPT(internal::concepts::Mutex) Mutex>
struct thunk_traits
{
    using mutex_type = Mutex;

    static constexpr unsigned retry_max = 10;
};

template <ESTD_CPP_CONCEPT(estd::concepts::v1::Bipbuf) Buf,
    ESTD_CPP_CONCEPT(internal::concepts::Mutex) Mutex = internal::noop_mutex>
class thunk;

}}  // namespace embr::inline sys::detail::inline v1

inline namespace v1 {

namespace layer1 {

template <unsigned sz, ESTD_CPP_CONCEPT(internal::concepts::Mutex) Mutex = internal::noop_mutex>
using thunk = sys::detail::v1::thunk<estd::layer1::bipbuf<sz>, Mutex>;

}

namespace layer3 {

template <ESTD_CPP_CONCEPT(internal::concepts::Mutex) Mutex = internal::noop_mutex>
using thunk = sys::detail::v1::thunk<estd::layer3::bipbuf, Mutex>;

}

}   // namespace embr::inline sys::inline v1

}}  // namespace embr::inline sys
#pragma once

#include "../internal/mutex.h"
#include "../internal/fwd.h"

namespace embr { inline namespace sys {

namespace detail { inline namespace v1 {

template <ESTD_CPP_CONCEPT(estd::concepts::v1::Bipbuf) Buf,
    ESTD_CPP_CONCEPT(internal::concepts::Mutex) Mutex = internal::noop_mutex>
class thunk;

}}  // namespace embr::inline sys::detail::inline v1

inline namespace v1 {

namespace layer1 {

template <unsigned sz, ESTD_CPP_CONCEPT(internal::concepts::Mutex) Mutex = internal::noop_mutex>
using thunk = sys::detail::v1::thunk<estd::layer1::bipbuf<sz>, Mutex>;

}

}   // namespace embr::inline sys::inline v1

}}  // namespace embr::inline sys
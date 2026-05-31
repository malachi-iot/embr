#pragma once

#include <estd/port/freertos/wrapper/task.h>

// Do not include this directly.  Instead, include `embr/thunk.h`

#include "../../fwd/thunk.h"

namespace embr { namespace freertos {

template <ESTD_CPP_CONCEPT(estd::concepts::v1::Bipbuf) Buf,
    ESTD_CPP_CONCEPT(internal::concepts::Mutex) Mutex,
    class F>
sys::detail::v1::thunk<Buf, Mutex>& operator <<(
    sys::detail::v1::thunk<Buf, Mutex>& out, F&& f)
{
    using thunk = sys::detail::v1::thunk<Buf, Mutex>;

    // DEBT: Rework detail::thunk to be traits-oriented so we can configure
    // retry# (10) and delay time
    out.post_with_retry(std::forward<F>(f),
        thunk::traits::retry_max, []{ vTaskDelay(1); });
    return out;
}

}}

namespace embr { inline namespace sys { namespace detail { inline namespace v1 {

// ADL assist
using freertos::operator <<;

}}}}
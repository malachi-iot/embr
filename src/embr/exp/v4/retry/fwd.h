#pragma once

namespace embr { namespace experimental { inline namespace v4 {

template <class Impl>
class Retry;

template <class Impl>
struct RetryItem;

// Perform additional asserts and integrity checks (catches more UB)
#ifndef FEATURE_EMBR_V4_RETRY_STRICT
#define FEATURE_EMBR_V4_RETRY_STRICT 1
#endif

}}}

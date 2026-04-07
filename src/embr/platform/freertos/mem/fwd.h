#pragma once

#include "../../../mem/v1/fwd.h"
#include "../../../mem/v1/functional/fwd.h"
#include "../../../mem/v1/vector/fwd.h"

#include "feature.h"

namespace embr { namespace mem { namespace freertos { inline namespace v1 {

namespace layer1 {

template <std::size_t N, std::size_t H>
class pool;

}

#if FEATURE_EMBR_GLOBAL_GC
using global_pool_type = freertos::layer1::pool<EMBR_GLOBAL_GC_STORAGE_SIZE, EMBR_GLOBAL_GC_HANDLE_SIZE>;

extern global_pool_type global_pool;

template <class T>
using vector = mem::v1::vector<T, global_pool_type, &global_pool>;

template <class F,
    template <class, estd::detail::impl::fn_options> class Impl = estd::detail::impl::function_virtual>
using function = mem::function<F, global_pool_type, &global_pool, Impl>;

template <class Signature>
using funclist = mem::v1::funclist<Signature, global_pool_type, &global_pool>;
#endif

}}}} // embr::mem:freertos::inline v1
#pragma once

#include <estd/mutex.h>

#include "../../../mem/v1/pool.h"
#include "../../../mem/v1/shared-handle.h"
#include "../../../mem/v1/unique-handle.h"


namespace embr { namespace mem { namespace freertos { inline namespace v1 {

namespace layer1 {

template <std::size_t N, std::size_t H>
class pool : public embr::mem::v1::layer1::pool<N, H, estd::freertos::mutex<true>>
{
    using mutex_type = estd::freertos::mutex<true>;
    using base_type = embr::mem::v1::layer1::pool<N, H, mutex_type>;

public:
    // Not available until estd v0.8.11 beta2
    //pool() : base_type(estd::in_place_type_t<mutex_type>{}, estd::defer_init_t{}) {}

    estd::errc init() { return estd::errc::not_supported; }
};

}

// From IDF (TBD)
#if CONFIG_EMBR_GLOBAL_GC_STORAGE_SZ
#endif

#ifndef FEATURE_EMBR_GLOBAL_GC
#define FEATURE_EMBR_GLOBAL_GC 1
#define EMBR_GLOBAL_GC_STORAGE_SIZE 512
#define EMBR_GLOBAL_GC_HANDLE_SIZE 8
#endif


#if FEATURE_EMBR_GLOBAL_GC
using global_pool_type = freertos::layer1::pool<EMBR_GLOBAL_GC_STORAGE_SIZE, EMBR_GLOBAL_GC_HANDLE_SIZE>;
extern global_pool_type global_pool;

template <class T>
using shared_handle = shared_handle<T, global_pool_type, &global_pool>;

using lock_handle = detail::v1::lock_handle<global_pool_type, &global_pool>;

template <class T, class ...Args>
shared_handle<T> make_shared(Args&&...args)
{
    return shared_handle<T>{ global_pool.template construct<T>(std::forward<Args>(args)...) };
}

template <class F>
using function = mem::function<F, global_pool_type, &global_pool>;

static_assert(sizeof(void*) >= sizeof(shared_handle<int[32]>));
#endif

}}}}

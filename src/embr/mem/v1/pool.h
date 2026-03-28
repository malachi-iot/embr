#pragma once

#include <estd/internal/macro/c++/ctor.h>
#include <estd/internal/size.h>

#include "../../internal/mutex.h"

#include "bundle.h"
#include "error.h"
#include "fwd.h"
#include "block.h"
#include "handles.h"
#include "page.h"
#include "storage.h"
#include "traits.h"

#include "pool/crtp.h"
#include "pool/ops.h"

#if FEATURE_STD_OSTREAM
#include <iosfwd>
#endif


namespace embr { namespace mem {

inline namespace v1 {

namespace layer1 {

template <std::size_t N, std::size_t H, class Mutex = void>
class pool : public detail::v1::pool_mutex_crtp<pool<N, H, Mutex>, Mutex>
{
    using mutex_base_type = detail::v1::pool_mutex_crtp<pool, Mutex>;

    friend class detail::v1::pool_crtp<pool>;
    friend class detail::v1::pool_mutex_crtp<pool, Mutex>;

    template <class Pool, Pool*>
    friend class detail::lock_handle;

public:
    using page_type = detail::v1::page<uint16_t>;
    using handles_traits = detail::v1::handles_traits<page_type[H]>;
    using handle_type = typename handles_traits::handle_type;
    using pool_traits = detail::v1::pool_traits<char[N]>;
    using pool_op_traits = detail::v1::pool_ops_traits<pool_traits, handles_traits>;
    using ops_type = detail::v1::pool_ops<pool_op_traits>;

private:
#if UNIT_TESTING
public:
#endif
    ops_type ops_;
    ops_type& ops() { return ops_; }

public:
    const ops_type& ops() const { return ops_; }

    ESTD_CPP_CONSTEXPR(14) pool()
    {
        ops_.reset();
    }

    template <class ...Args>
    ESTD_CPP_CONSTEXPR(14) explicit pool(estd::in_place_type_t<Mutex>, Args&&...args) :
        mutex_base_type(
            estd::in_place_type_t<Mutex>{},
            std::forward<Args>(args)...)
    {
        ops_.reset();
    }
};

}


namespace layer3 {

class pool : public detail::v1::pool_mutex_crtp<pool>
{
    template <class Pool, Pool*>
    friend class detail::lock_handle;

public:
    using page_type = detail::v1::page<uint16_t>;
    using handles_traits = detail::v1::handles_traits<estd::span<page_type>>;
    using pool_traits = detail::v1::pool_traits<estd::span<char>>;
    using handle_type = typename handles_traits::handle_type;
    using pool_op_traits = detail::v1::pool_ops_traits<pool_traits, handles_traits>;
    using ops_type = detail::v1::pool_ops<pool_op_traits>;

private:
#if UNIT_TESTING
public:
#endif
    ops_type ops_;
    ops_type& ops() { return ops_; }

public:
    const ops_type& ops() const { return ops_; }

    ESTD_CPP_CONSTEXPR(14) pool(
        const estd::span<page_type>& pages,
        const estd::span<char>& raw) :
        ops_(raw, pages)
    {
        ops_.reset();
    }
};

}

}

}}

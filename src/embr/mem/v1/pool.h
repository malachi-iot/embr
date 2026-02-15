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
#include "traits.h"

#include "pool/crtp.h"
#include "pool/ops.h"

#if FEATURE_STD_OSTREAM
#include <iosfwd>
#endif


namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class Traits>
class pool : public Traits
{
    using this_type = pool;

public:
    using traits = Traits;
    using block_type = typename traits::block;
    using container_type = typename traits::type;
    //using typename traits::page_type;
    //using typename traits::pos_type;
    //using traits::data;

    // Doesn't work for span, see https://github.com/malachi-iot/estdlib/issues/167
    using iterator_traits = estd::iterator_traits<container_type>;

protected:
    template <class Traits2>
    friend class pool_ops;

    //static_assert(sizeof(typename iterator_traits::value_type) == 1);

    container_type pool_;

    ESTD_CPP_CONSTEXPR(14) block_type* block(const bytes_unit<unsigned>& at)
    {
        //assert(at.count() != page_type::null);
        return reinterpret_cast<block_type*>(estd::data(pool_) + at.count());
    }

    ESTD_CPP_CONSTEXPR(14) const block_type* block(const bytes_unit<unsigned>& at) const
    {
        return reinterpret_cast<const block_type*>(estd::data(pool_) + at.count());
    }

/*
#if PAGE_ALIAS
    template <class Rep, unsigned alias>
    v1::block* block(const v1::page<Rep, alias>& page)
#else
    template <class Rep, class Ratio>
    v1::block* block(const v1::page<Rep, Ratio>& page)
#endif
    {
        return block(page.pos());
    } */

public:
    template <class ...Args>
    explicit constexpr pool(Args&&...args) :
        pool_(std::forward<Args>(args)...)
    {
        // DEBT: Unhardcode this guy
        constexpr unsigned aliasing = 8;

        assert(estd::size(pool_) % aliasing == 0);
    }

    // Just for diagnostics
    const char* data() const { return estd::data(pool_); }
};

// EXPERIMENTAL
template <class HandlesContainer, class PoolContainer>
class pool_aggregate
{
protected:
    using handles_traits = v1::handles_traits<HandlesContainer>;
    using page_type = typename handles_traits::value_type;
    detail::v1::pool<detail::v1::pool_traits<PoolContainer>> pool_;
    detail::v1::handles<handles_traits> handles_;
};

}}

inline namespace v1 {

namespace layer1 {

template <std::size_t N, std::size_t H, class Mutex = void>
class pool :
    public detail::v1::pool_crtp<pool<N, H, Mutex>>,
    public detail::v1::pool_mutex_crtp<pool<N, H, Mutex>, Mutex>
{
    using mutex_base_type = detail::v1::pool_mutex_crtp<pool, Mutex>;

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

class pool :
    public detail::v1::pool_crtp<pool>,
    public detail::v1::pool_mutex_crtp<pool>
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

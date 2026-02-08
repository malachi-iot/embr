#pragma once

#include <estd/internal/macro/c++/ctor.h>
#include <estd/internal/size.h>
#include <estd/mutex.h>

#include "../../internal/mutex.h"

#include "bundle.h"
#include "error.h"
#include "fwd.h"
#include "block.h"
#include "handles.h"
#include "page.h"
#include "traits.h"

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

#pragma push_macro("THIS")
#pragma push_macro("OPS")
#undef THIS
#undef OPS
#define THIS static_cast<Derived*>(this)
#define OPS THIS->ops()


// DEBT: I was resisting an ebo/mutex() paradigm but that's starting to look like
// the best option
template <class Derived, class Mutex = void>
class pool_mutex_crtp
{
protected:
    Mutex mutex_;

    template <class ...Args>
    constexpr explicit pool_mutex_crtp(estd::in_place_type_t<Mutex>, Args&&...args) :
        mutex_{std::forward<Args>(args)...}
    {

    }

    constexpr pool_mutex_crtp() = default;

public:
    using mutex_type = Mutex;

    template <class Handle>
    void* lock(Handle h)
    {
        return OPS.lock(h, mutex_);
    }

    template <class Handle>
    void unlock(Handle h)
    {
        OPS.unlock(h, mutex_);
    }


    // Only for trivial mode, use construct otherwise
    template <class Derived2 = Derived>
    typename Derived2::handle_type alloc(unsigned sz)
    {
        estd::lock_guard<Mutex> lg(mutex_);

        constexpr auto mode = v1::block_mode_enum::Trivial;
        // DEBT: Uncouple from block_8
        constexpr unsigned block_sz = v1::block_8::header_size(mode).count();
        // DEBT: Probably do_alias ought to be in page_type
        return OPS.alloc(OPS.do_alias(sz + block_sz), mode).handle;
    }


    template <class T, class ...Args, class Derived2 = Derived>
    typename Derived2::handle_type construct(Args&&...args)
    {
        estd::lock_guard<Mutex> lg(mutex_);

        return detail::construct<T>(OPS.storage_, OPS.handles_, std::forward<Args>(args)...);
    }


    void dealloc(int h)
    {
        estd::lock_guard<Mutex> lg(mutex_);

        OPS.dealloc(h);
    }

    // Higher thresh = demand quicker, easier GC
    int gc(int thresh = 0)
    {
        using ops_type = typename Derived::ops_type;
        using fragmentation = typename ops_type::fragmentation;
        fragmentation frag;
        const typename fragmentation::candidate& cand = frag.candidates[0];

        estd::lock_guard<Mutex> lg(mutex_);

        OPS.assess(&frag);

        if(cand.score > 0)
        {
            if(cand.score > thresh)
                OPS.defrag(cand);
        }

        return cand.score;
    }
};

template <class Derived>
class pool_mutex_crtp<Derived>
{
public:
    using mutex_type = void;

    template <class Handle>
    void* lock(Handle h)
    {
        return OPS.lock(h);
    }

    template <class Handle>
    void unlock(Handle h)
    {
        OPS.unlock(h);
    }


    template <class T, class ...Args, class Derived2 = Derived>
    typename Derived2::handle_type construct(Args&&...args)
    {
        return detail::construct<T>(OPS.storage_, OPS.handles_, std::forward<Args>(args)...);
    }

    // Only for trivial mode, use construct otherwise
    template <class Derived2 = Derived>
    typename Derived2::handle_type alloc(unsigned sz)
    {
        constexpr auto mode = v1::block_mode_enum::Trivial;
        // DEBT: Uncouple from block_8
        constexpr unsigned block_sz = v1::block_8::header_size(mode).count();
        return OPS.alloc(OPS.do_alias(sz + block_sz), mode).handle;
    }


    void dealloc(int h)
    {
        OPS.dealloc(h);
    }


    /*
    template <class Handle>
    void realloc(Handle h, unsigned sz)
    {
        // FIX: Not complete
        OPS.realloc(OPS.get_bundle(h), OPS.do_alias(sz));
    }   */
};



template <class Derived>
class pool_crtp
{
    //using handle_type = typename Derived::handle_type;

public:
    bool realloc(int h, unsigned size)
    {
        auto bn = OPS.get_bundle(h);
        // DEBT: remove explicit bytes_unit, dependent on https://github.com/malachi-iot/estdlib/issues/173
        return OPS.realloc(bn, bn.header_size() + bytes_unit<unsigned>(size));
    }

    void reset()
    {
        OPS.reset();
    }

    unsigned allocated()
    {
        return OPS.alloced().count();
    }
};

#pragma pop_macro("OPS")
#pragma pop_macro("THIS")

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

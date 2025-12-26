#pragma once

#include <estd/cstdint.h>
#include <estd/internal/macro/c++/ctor.h>
#include <estd/utility.h>

#include "fwd.h"
#include "block.h"
#include "handles.h"
#include "page.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

// 16DEC25 MB - boilerplate for incoming playground.memory mem-11 formalization

template <class Container>
struct pool_traits : container_traits<Container>
{
    // DEBT: My gut tells me page_type/pos_type has a better home than this traits
    using page_type = page<uint16_t>;
    using pos_type = page_type::unit_type;
};

template <class Traits>
class pool : public Traits
{
    using this_type = pool;

public:
    using traits = Traits;
    using typename traits::container_type;
    using typename traits::page_type;
    using typename traits::pos_type;
    //using traits::data;

protected:

    container_type pool_;

    v1::block* block(pos_type at)
    {
        return reinterpret_cast<v1::block*>(std::data(pool_) + page_unit_type(at).count());
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

    static constexpr unsigned aliasing = pos_type::period::num;

    // DEBT: Need better name
    static constexpr pos_type do_alias(unsigned v)
    {
        return pos_type((v + aliasing - 1) / aliasing);
    }


#if UNIT_TESTING
public:
#endif
    template <class HandlesTraits>
    struct ops //: HandlesTraits    // FIX: We ought to be able to do this, what's stopping us?
    {
        using block = v1::block;
        //using bundle = v1::bundle_base<HandlesTraits, page_type>;
        using bundle = v1::bundle;
        using traits = HandlesTraits;
        using handle_type = typename traits::size_type;
        using handles_type = handles<traits>;
        using page_type = typename traits::value_type;

        static constexpr unsigned aliasing = pos_type::period::num;

        this_type& self_;
        handles_type& handles_;

        // Best to have this guy here and not in above pool so that we make no assumptions
        // about page_type and handle_type
        bundle get_bundle(page_type& page, handle_type handle) const
        {
            return { self_.block(page.pos()), &page, handle };
        }

        bundle get_bundle(handle_type h) const
        {
            return get_bundle(handles_[h], h);
        }

        bundle get_bundle(page_type& page) const
        {
            return get_bundle(page, &page - &handles_[0]);
        }

        block* create_free_block(pos_type, handle_type prev, handle_type next);

        /// Creates a new free block inside bundle, before next block
        /// @param at
        /// @details presumes bundle is big enough to split, no checks performed.
        /// Does not notice if block following this bundle is already free
        handle_type split(bundle, pos_type at);

        ///
        /// @brief merge
        /// @param current
        /// @param next MUST be a free block, returns false otherwise
        /// @details
        bool merge(bundle current, bundle next);

        void move(bundle from, bundle to, unsigned logical_sz);

        bundle first_free(pos_type phys_sz, pos_type* found_size) const;

        bundle prev(const block*) const;
        bundle prev(const bundle& bn) const { return prev(bn.block); }
        bundle next(const block*) const;
        bundle next(const bundle& bn) const { return next(bn.block); }

        pos_type phys_size(const bundle&) const;
        pos_type phys_size(int h, page_type& p) const
        {
            return phys_size(self_.bundle(p, h));
        }

        unsigned logical_size(const bundle&) const;

        template <block::modes mode>
        bundle alloc(pos_type phys_sz);

        template <block::modes mode, class T, class ...Args>
        bundle construct(Args&&...);

        void dealloc(bundle);
        void dealloc(handle_type h) { dealloc(get_bundle(handles_[h], h)); }

        void reset();

        void* lock(handle_type h);
        void unlock(handle_type h);

        void ref_up(handle_type h);
        void ref_down(handle_type h);

        unsigned alloced() const;
        unsigned available() const;
    };

public:
    ESTD_CPP_FORWARDING_CTOR_MEMBER(pool, pool_)

    // Just for diagnostics
    const char* data() const { return std::data(pool_); }

    using handle_type = int;

    template <v1::block::modes mode = v1::block::Trivial, class Traits2>
    typename Traits2::size_type alloc(handles<Traits2>& h, unsigned logical_sz)
    {
        constexpr unsigned block_sz = v1::block::header_size<mode>();
        return ops<Traits2>{*this, h}.template alloc<mode>(do_alias(logical_sz + block_sz)).handle;
    }

    template <class T, class Traits2, class ...Args>
    typename Traits2::size_type construct(handles<Traits2>& h, Args&&...args);

    template <class Traits2>
    void dealloc(v1::handles<Traits2>& handles, typename Traits2::size_type h)
    {
        return ops<Traits2>{*this, handles}.dealloc(h);
    }

    template <class Traits2>
    void reset(v1::handles<Traits2>& handles)
    {
        ops<Traits2>{*this, handles}.reset();
    }
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

template <class Derived>
class pool_crtp
{
    //using handle_type = typename Derived::handle_type;

public:
    void* lock(int h)
    {
        return OPS.lock(h);
    }

    void unlock(int h)
    {
        OPS.unlock(h);
    }

    void dealloc(int h)
    {
        OPS.dealloc(h);
    }
};

#pragma pop_macro("OPS")
#pragma pop_macro("THIS")

}}

inline namespace v1 {

namespace layer1 {

template <std::size_t N, std::size_t H>
class pool : public detail::v1::pool_crtp<pool<N, H>>
{
    friend class detail::v1::pool_crtp<pool<N, H>>;

public:
    using page_type = detail::v1::page<uint16_t>;
    using handles_traits = detail::v1::handles_traits<page_type[H]>;
    using pool_traits = detail::v1::pool_traits<char[N]>;

private:
    detail::v1::pool<pool_traits> pool_;
    detail::v1::handles<handles_traits> handles_;

#if UNIT_TESTING
public:
#endif
    using ops_type = typename detail::v1::pool<pool_traits>::template ops<handles_traits>;

    ops_type ops() { return {pool_, handles_}; }

public:
    using handle_type = typename handles_traits::size_type;

    handle_type alloc(int logical_sz) { return pool_.alloc(handles_, logical_sz); }

    template <class T, class ...Args>
    handle_type construct(Args&&...args)
    {
        return pool_.template construct<T>(handles_, std::forward<Args>(args)...);
    }
};

}

}

}}

#pragma once

#include <estd/cstdint.h>
#include <estd/internal/macro/c++/ctor.h>
#include <estd/string_view.h>
#include <estd/utility.h>

#include "bundle.h"
#include "error.h"
#include "fwd.h"
#include "block.h"
#include "handles.h"
#include "page.h"

#if FEATURE_STD_OSTREAM
#include <iosfwd>
#endif

namespace embr { namespace mem {

namespace detail { inline namespace v1 {


struct fragmentation
{
    struct candidate
    {
        int score;
        const_bundle bundle;
        const_bundle move_to;
        bool overlap{true};
        bool adjacent{true};

        constexpr bool invariant() const
        {
            return move_to.allocated() == false && bundle.allocated();
        }
    };

    candidate candidates[2];
};

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
        //assert(at.count() != page_type::null);
        const unsigned offset = page_unit_type(at).count();
        return reinterpret_cast<v1::block*>(std::data(pool_) + offset);
    }

    const v1::block* block(pos_type at) const
    {
        return reinterpret_cast<const v1::block*>(std::data(pool_) + page_unit_type(at).count());
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

        const_bundle get_bundle(const page_type& page, handle_type handle) const
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

        const_bundle get_bundle(const page_type& page) const
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

        /// Same as merge but 'current' must be free also
        /// @brief merge_free
        /// @param current
        /// @param next
        /// @return
        ///
        bool merge_if_free(bundle current, bundle next);

        void move(bundle from, bundle to, unsigned logical_sz, bool is_overlapping = false);

        bundle first_free(pos_type phys_sz, pos_type* found_size) const;

        template <class Traits2, class Block, class Page>
        void prev(const block*, bundle_base<Traits2, Block, Page>* out) const;

        bundle prev(const block*) const;
        bundle prev(const bundle& bn) const { return prev(bn.block); }

        template <class Traits2, class Block, class Page>
        void next(const block*, bundle_base<Traits2, Block, Page>* out) const;

        bundle next(const block*) const;
        template <class Traits2, class Block, class Page>
        bundle next(const bundle_base<Traits2, Block, Page>& bn) const { return next(bn.block); }

        template <class Traits2, class Block, class Page>
        pos_type phys_size(const bundle_base<Traits2, Block, Page>&) const;
        pos_type phys_size(int h, page_type& p) const
        {
            return phys_size(self_.bundle(p, h));
        }

        unsigned logical_size(const bundle&) const;

        /// Moves block itself to new location - does not consider payload data
        /// Free blocks only is RECOMMENDED
        block* move_block(page_type&, pos_type);

        /// Resizes a bundle to new presented size.  next block MUST be a free block
        /// with enough space
        /// @brief resize
        /// @param new_sz
        /// @return
        v1::block* resize(bundle, pos_type new_sz);

        bundle alloc(pos_type phys_sz, block::modes mode);

        template <block::modes mode, class T, class ...Args>
        bundle construct(Args&&...);

        void dealloc(bundle);
        void dealloc(handle_type h) { dealloc(get_bundle(handles_[h], h)); }

        void reset();

        void* lock(bundle);
        void* lock(handle_type h)   { return lock(get_bundle(h)); }
        void unlock(handle_type h);

        void ref_up(handle_type h);
        void ref_down(handle_type h);

        unsigned alloced() const;
        unsigned available() const;

        void assess(fragmentation*) const;
        void defrag(const fragmentation::candidate&);

        invariant_result invariant() const;

        // Diagnostic dump of pool content
        std::ostream& dump(std::ostream& out) const;
    };

public:
    ESTD_CPP_FORWARDING_CTOR_MEMBER(pool, pool_)

    // Just for diagnostics
    const char* data() const { return std::data(pool_); }

    using handle_type = int;

    template <v1::block::modes mode = v1::block::Trivial, class Traits2>
    typename Traits2::size_type alloc(handles<Traits2>& h, unsigned logical_sz)
    {
        constexpr unsigned block_sz = v1::block::header_size(mode);
        return ops<Traits2>{*this, h}.alloc(do_alias(logical_sz + block_sz), mode).handle;
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

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

#if PAGE_ALIAS
    template <class Rep, unsigned alias>
    v1::block* block(const v1::page<Rep, alias>& page)
#else
    template <class Rep, class Ratio>
    v1::block* block(const v1::page<Rep, Ratio>& page)
#endif
    {
        return reinterpret_cast<v1::block*>(std::data(pool_) + page_unit_type(page.pos()).count());
    }

    v1::bundle bundle(page_type& page, unsigned handle)
    {
        return { block(page), &page, handle };
    }

#if UNIT_TESTING
public:
#endif
    template <class HandlesTraits>
    struct ops //: HandlesTraits    // FIX: We ought to be able to do this, what's stopping us?
    {
        using block = v1::block;
        using bundle = v1::bundle;
        using traits = HandlesTraits;
        using handle_type = typename traits::size_type;
        using handles_type = handles<traits>;

        static constexpr unsigned aliasing = pos_type::period::num;

        this_type& self_;
        handles_type& handles_;

        /// Creates a new free block inside bundle, before next block
        /// @param at
        /// @details presumes bundle is big enough to split, no checks performed.
        /// Does not notice if block following this bundle is already free
        handle_type split(bundle, pos_type at);

        void merge(bundle current, bundle next);

        bundle first_free(pos_type phys_sz, pos_type* found_size) const;

        bundle next(const v1::block*) const;
        bundle next(const v1::bundle& bn) const { return next(bn.block); }

        pos_type phys_size(const v1::bundle&) const;
        pos_type phys_size(int h, page_type& p) const
        {
            return phys_size(self_.bundle(p, h));
        }

        template <v1::block::modes mode>
        handle_type alloc_old(unsigned phys_sz, v1::bundle*);

        template <block::modes mode>
        bundle alloc_new(pos_type phys_sz);

        template <block::modes mode, class T, class ...Args>
        handle_type construct(bundle*, Args&&...);

        void dealloc(handle_type);

        void reset();
    };

public:
    ESTD_CPP_FORWARDING_CTOR_MEMBER(pool, pool_)

    using handle_type = int;

    template <class Traits2>
    typename Traits2::size_type alloc(handles<Traits2>& h, unsigned logical_sz, unsigned block_sz)
    {
        v1::bundle bn;
        return ops<Traits2>{*this, h}.template alloc_old<v1::block::Trivial>(logical_sz + block_sz, &bn);
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

}}

inline namespace v1 {


}

}}

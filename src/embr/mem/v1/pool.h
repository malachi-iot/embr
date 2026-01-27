#pragma once

#include <estd/cstdint.h>
#include <estd/internal/macro/c++/ctor.h>
#include <estd/internal/size.h>
#include <estd/string_view.h>
#include <estd/utility.h>

#include "bundle.h"
#include "error.h"
#include "fwd.h"
#include "block.h"
#include "handles.h"
#include "page.h"
#include "traits.h"

#if FEATURE_STD_OSTREAM
#include <iosfwd>
#endif


namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class Bundle>
struct fragmentation_base
{
    int largest_free_handle;

    struct candidate
    {
        int score;
        Bundle bundle;
        Bundle move_to;
        bool overlap{true};
        bool adjacent{true};

        constexpr bool invariant() const
        {
            return move_to.allocated() == false && bundle.allocated();
        }
    };

    candidate candidates[2];
};

template <class T, class PoolTraits, class HandlesTraits, class ...Args>
typename HandlesTraits::size_type construct(pool<PoolTraits>& p, handles<HandlesTraits>& h, Args&&...args);

// TODO: Do 'concepts'
// 23JAN26 MB Appears unavoidable to template it out to this level.  Was hoping CRTP wizardry would help us, but I don't
// like an intermediate pool::ops with pointer/references out to the real classes.  That makes the optimizer work a lot
// harder and aside from intellisense doesn't make the code easier to read
template <class Traits>
class pool_ops
{
public:
    using traits = Traits;
    using storage_type = estd::remove_reference_t<typename traits::pool_type>;
    using handles_type = estd::remove_reference_t<typename traits::handles_type>;
    using pool_traits = typename storage_type::traits;
    using handles_traits = typename handles_type::traits;
    using block = v1::block;
    using handle_type = typename handles_traits::size_type;
    using page_type = typename handles_traits::value_type;
    using pos_type = typename page_type::unit_type;
    using bundle = v1::bundle_base<handles_traits, block>;
    using const_bundle = v1::bundle_base<handles_traits, const block>;
    using bytes = bytes_unit<unsigned>;

    template <class Derived>
    friend class pool_crtp;

protected:

    typename traits::pool_type self_;
    typename traits::handles_type handles_;

public:
    // DEBT: Pool gets punished a little with just one init parameter, I think we'll be OK though
    template <class PoolArg, class ...HandlesArgs>
    constexpr pool_ops(PoolArg&& pa, HandlesArgs&&...ha) :
        self_{std::forward<PoolArg>(pa)},
        handles_{std::forward<HandlesArgs>(ha)...}
    {}

    // DEBT: More clumsiness, this one skips pool init altogether
    template <class ...HandlesArgs>
    constexpr pool_ops(estd::nullopt_t, HandlesArgs&&...ha) :
        handles_{std::forward<HandlesArgs>(ha)...}
    {}

    constexpr pool_ops() = default;

    constexpr const storage_type& storage() const { return self_; }
    constexpr const handles_type& handles() const { return handles_; }

    static constexpr unsigned aliasing = pos_type::period::num;

    block* create_free_block(pos_type, handle_type prev, handle_type next);

    /// Low level alloc TBD docs
    void alloc(bundle, pos_type found_sz, pos_type phys_sz, block::modes);

    bundle alloc(pos_type phys_sz, block::modes mode);

    template <block::modes mode, class T, class ...Args>
    bundle construct(Args&&...);

    void dealloc(bundle);
    void dealloc(handle_type h) { dealloc(get_bundle(handles_[h], h)); }

    using fragmentation = fragmentation_base<const_bundle>;

    void assess(fragmentation*) const;
    void defrag(const typename fragmentation::candidate&, bool relink = true);

    invariant_result invariant() const;

    // Diagnostic dump of pool content
    std::ostream& dump(std::ostream& out) const;

    const_bundle first() const;

    const_bundle first_free(pos_type phys_sz, pos_type* found_size) const;

    // Best to have this guy here and not in above pool so that we make no assumptions
    // about page_type and handle_type
    const_bundle get_bundle(const page_type& page, handle_type handle) const
    {
        return { self_.block(page.pos()), &page, handle };
    }

    const_bundle get_bundle(handle_type h) const
    {
        return get_bundle(handles_[h], h);
    }

    const_bundle get_bundle(const page_type& page) const
    {
        return get_bundle(page, &page - &handles_[0]);
    }

    template <class ...Args>
    bundle get_bundle(Args&&...args)
    {
        return std::as_const(*this).get_bundle(std::forward<Args>(args)...).unconst();
    }

    // DEBT: Need better name
    static constexpr pos_type do_alias(unsigned v)
    {
        return pos_type((v + aliasing - 1) / aliasing);
    }

    void* lock(bundle);
    void* lock(handle_type h)   { return lock(get_bundle(h)); }
    void unlock(handle_type h);

    template <class Block>
    void prev(Block*, bundle_base<handles_traits, Block>* out) const;

    bundle prev(const const_bundle& bn) { return get_bundle(bn.block->prev()); }
    const_bundle prev(const const_bundle& bn) const { return get_bundle(bn.block->prev()); }

    template <class Block>
    void next(Block*, bundle_base<handles_traits, Block>* out) const;

    bundle next(const const_bundle& bn) { return get_bundle(bn.block->next()); }
    const_bundle next(const const_bundle& bn) const { return get_bundle(bn.block->next()); }

    static estd::units::bytes<unsigned> logical_size(block::modes, pos_type phys_sz);

    unsigned logical_size(const const_bundle&) const;

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
    bool merge_if_free(bundle current, bundle next);

    ///
    /// @brief Moves an allocated block to a new free block location.  Updates block metadata so 'to' block becomes allocated
    /// @param from
    /// @param to free block
    /// @param logical_sz feeds block mover +
    /// @param desired_logical_sz indicates logical realloc resize
    /// @param is_overlapping
    /// @remarks to->next may change due to split operation.  logical_sz is NOT checked for
    /// overflow
    validated_result move(bundle from, bundle to, unsigned logical_sz,
        unsigned desired_logical_sz,
        bool is_overlapping);


    /// Moves block itself to new location - does not consider payload data
    /// Free blocks only is RECOMMENDED
    block* move_block(page_type&, pos_type);

    pos_type phys_size(const const_bundle&) const;

    /// Grows or shrinks existing allocation, possibly moving it and others around
    /// @brief realloc
    /// @param new_sz
    /// @return true on success, false on failure (no suitable free block found)
    bool realloc(bundle, pos_type new_sz);

    void ref_down(handle_type h);
    void ref_up(handle_type h);

    void reset();

    /// Resizes a bundle to new presented size.  next block MUST be a free block
    /// with enough space
    /// @brief resize
    /// @param new_sz
    /// @return
    v1::block* resize(bundle, pos_type new_sz);
    block* resize(bundle bn, bundle bn_next, pos_type new_sz);

    /// Creates a new free block inside bundle, before next block
    /// @param at particular location of split - NOT a size
    /// @details presumes bundle is big enough to split, no checks performed.
    /// @returns handle of new free block created
    /// Does not notice if block following this bundle is already free.  Renamed
    /// from 'split' to disambiguate that we need absolute position, not relative size
    handle_type split_at(bundle, pos_type at);

    ///
    /// @brief virtual_swap performs page position swap and relinks to maintain contiguous next and prev
    /// @param lhs
    /// @param rhs
    void virtual_swap(bundle& lhs, bundle& rhs);

    ///
    /// @brief virtual_move similar to virtual_swap, but only remaps a one page position - does do relinking
    /// @param from
    /// @param to - a null page
    void virtual_move(bundle& from, bundle& to);

    // Reports in logical size
    // DEBT: Report back in 1:1 pos_type since ops is lower level and "logical" isn't in title
    bytes alloced() const;
    bytes available() const;
};

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

    block_type* block(bytes_unit<unsigned> at)
    {
        //assert(at.count() != page_type::null);
        return reinterpret_cast<block_type*>(estd::data(pool_) + at.count());
    }

    const block_type* block(bytes_unit<unsigned> at) const
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

    //using handle_type = int;

    template <v1::block::modes mode = v1::block::Trivial, class Traits2>
    typename Traits2::size_type alloc(handles<Traits2>& h, unsigned logical_sz)
    {
        constexpr unsigned block_sz = v1::block::header_size(mode).count();
        using ops_type = pool_ops<pool_ops_ref_traits<pool, handles<Traits2>>>;
        return ops_type{*this, h}.alloc(ops_type::do_alias(logical_sz + block_sz), mode).handle;
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

    // Only for trivial mode, use construct otherwise
    // UNTESTED
    template <class Derived2 = Derived>
    typename Derived2::handle_type alloc(unsigned sz)
    {
        constexpr auto mode = v1::block::Trivial;
        constexpr unsigned block_sz = v1::block::header_size(mode).count();
        return OPS.alloc(OPS.do_alias(sz + block_sz), mode).handle;
    }

    void realloc(int h, unsigned size)
    {
        auto bn = OPS.get_bundle(h);
        // DEBT: remove explicit bytes_unit, dependent on https://github.com/malachi-iot/estdlib/issues/173
        OPS.realloc(bn, bn.header_size() + bytes_unit<unsigned>(size));
    }

    void reset()
    {
        OPS.reset();
    }

    unsigned allocated()
    {
        return OPS.alloced().count();
    }

    template <class T, class ...Args, class Derived2 = Derived>
    typename Derived2::handle_type construct(Args&&...args)
    {
        return detail::construct<T>(OPS.self_, OPS.handles_, std::forward<Args>(args)...);
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
    using handle_type = typename handles_traits::size_type;
    using pool_traits = detail::v1::pool_traits<char[N]>;
    using pool_op_traits = detail::v1::pool_ops_traits<pool_traits, handles_traits>;
    using ops_type = detail::v1::pool_ops<pool_op_traits>;

private:
#if UNIT_TESTING
public:
#endif
    ops_type ops_;
    ops_type& ops() { return ops_; }
    const ops_type& ops() const { return ops_; }

public:
    //handle_type alloc(int logical_sz) { return pool_.alloc(handles_, logical_sz); }
};

}


namespace layer3 {

class pool : public detail::v1::pool_crtp<pool>
{
    friend class detail::v1::pool_crtp<pool>;

public:
    using page_type = detail::v1::page<uint16_t>;
    using handles_traits = detail::v1::handles_traits<estd::span<page_type>>;
    using pool_traits = detail::v1::pool_traits<estd::span<char>>;
    using handle_type = typename handles_traits::size_type;
    using pool_op_traits = detail::v1::pool_ops_traits<pool_traits, handles_traits>;
    using ops_type = detail::v1::pool_ops<pool_op_traits>;

private:
#if UNIT_TESTING
public:
#endif
    ops_type ops_;
    ops_type& ops() { return ops_; }
    const ops_type& ops() const { return ops_; }

public:
    pool(estd::span<page_type> pages, estd::span<char> raw) :
        ops_(raw, pages)
    {}

    //handle_type alloc(int logical_sz) { return pool_.alloc(handles_, logical_sz); }
};

}

}

}}

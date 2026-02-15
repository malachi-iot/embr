#pragma once

#include <estd/utility.h>

#include "../block.h"
#include "../unit.h"

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

// TODO: Do 'concepts'
// 23JAN26 MB Appears unavoidable to template it out to this level.  Was hoping CRTP wizardry would help us, but I don't
// like an intermediate pool::ops with pointer/references out to the real classes.  That makes the optimizer work a lot
// harder and aside from intellisense doesn't make the code easier to read
template <class Traits>
class pool_ops
{
public:
    using traits = Traits;
    using storage_type = estd::remove_reference_t<typename traits::storage_type>;
    using handles_type = estd::remove_reference_t<typename traits::handles_type>;
    using pool_traits = typename storage_type::traits;
    using handles_traits = typename handles_type::traits;
    // DEBT: Would prefer grabbing this from traits, but that interrupts convenience
    // of getting at enum
    using block = v1::block_8;
    using handle_type = typename handles_traits::handle_type;
    using page_type = typename handles_traits::value_type;
    using pos_type = typename page_type::unit_type;
    using bundle = v1::bundle_base<handles_traits, block>;
    using const_bundle = v1::bundle_base<handles_traits, const block>;
    using bytes = bytes_unit<unsigned>;

    template <class Derived>
    friend class pool_crtp;

    template <class Derived, class Mutex>
    friend class pool_mutex_crtp;

protected:

    typename traits::storage_type storage_;
    typename traits::handles_type handles_;

public:
    // DEBT: Pool gets punished a little with just one init parameter, I think we'll be OK though
    template <class PoolArg, class ...HandlesArgs>
    constexpr explicit pool_ops(PoolArg&& pa, HandlesArgs&&...ha) :
        storage_{std::forward<PoolArg>(pa)},
        handles_{std::forward<HandlesArgs>(ha)...}
    {}

    // DEBT: More clumsiness, this one skips pool init altogether
    template <class ...HandlesArgs>
    constexpr explicit pool_ops(estd::nullopt_t, HandlesArgs&&...ha) :
        handles_{std::forward<HandlesArgs>(ha)...}
    {}

    constexpr pool_ops() = default;

    constexpr const storage_type& storage() const { return storage_; }
    constexpr const handles_type& handles() const { return handles_; }

    static constexpr unsigned aliasing = pos_type::period::num;

    ESTD_CPP_CONSTEXPR(14) block* create_free_block(pos_type, handle_type prev, handle_type next);

    /// Low level alloc TBD docs
    void alloc(const bundle&, pos_type found_sz, pos_type phys_sz, block::modes);

    bundle alloc(pos_type phys_sz, block::modes mode);

    template <block::modes mode, class T, class ...Args>
    bundle construct(Args&&...);

    bundle copy(bundle);

    void dealloc(bundle);
    void dealloc(handle_type h) { dealloc(get_bundle(handles_[h], h)); }

    using fragmentation = fragmentation_base<const_bundle>;

    void assess(fragmentation*) const;
    void defrag(const typename fragmentation::candidate&, bool relink = true);

    invariant_result invariant() const;

#if FEATURE_STD_OSTREAM
    // Diagnostic dump of pool content
    std::ostream& dump(std::ostream& out) const;
#endif

    const_bundle first() const;

    const_bundle first_free(pos_type phys_sz, pos_type* found_size) const;

    // Best to have this guy here and not in above pool so that we make no assumptions
    // about page_type and handle_type
    const_bundle get_bundle(const page_type& page, handle_type handle) const
    {
        return { storage_.block(page.pos()), &page, handle };
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
        return estd::as_const(*this).get_bundle(std::forward<Args>(args)...).unconst();
    }

    // DEBT: Need better name
    static constexpr pos_type do_alias(unsigned v)
    {
        return pos_type((v + aliasing - 1) / aliasing);
    }

    void* lock(bundle);

    template <class Mutex = embr::internal::noop_mutex>
    void* lock(handle_type h, Mutex mutex = {});

    template <class Mutex = embr::internal::noop_mutex>
    void unlock(handle_type h, Mutex mutex = {});

    bundle prev(const const_bundle& bn) { return get_bundle(bn.block->prev()); }
    const_bundle prev(const const_bundle& bn) const { return get_bundle(bn.block->prev()); }

    template <class Block>
    void next(Block*, bundle_base<handles_traits, Block>* out) const;

    bundle next(const const_bundle& bn) { return get_bundle(bn.block->next()); }
    const_bundle next(const const_bundle& bn) const { return get_bundle(bn.block->next()); }

    static estd::units::bytes<unsigned> logical_size(block::modes, pos_type phys_sz);

    estd::units::bytes<unsigned> logical_size(const const_bundle&) const;

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

    ESTD_CPP_CONSTEXPR(14) void reset();

    /// Resizes a bundle to new presented size.  next block MUST be a free block
    /// with enough space.  No realloc occurs here ever
    /// @brief resize
    /// @param new_sz
    /// @return
    /// @remarks low-level call, consumers want realloc
    block* resize(bundle, pos_type new_sz);
    block* resize(bundle bn, bundle bn_next, pos_type new_sz);

    /// Creates a new free block inside bundle, before next block
    /// @param at particular location of split - NOT a size
    /// @details presumes bundle is big enough to split, no checks performed.
    /// @returns handle of new free block created
    /// Does not notice if block following this bundle is already free.  Renamed
    /// from 'split' to disambiguate that we need absolute position, not relative size
    handle_type split_at(const bundle&, pos_type at);

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

}}

}}

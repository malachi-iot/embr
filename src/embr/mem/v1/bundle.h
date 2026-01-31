#pragma once

#include "block.h"
#include "enum.h"
#include "error.h"
#include "fwd.h"
#include "page.h"
#include "traits.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class HandlesTraits, class Block>
struct bundle_base : block_mode_enum
{
    using handles_traits = HandlesTraits;
    using handle_type = typename HandlesTraits::size_type;
    using page_noncv_type = typename HandlesTraits::value_type;
    using page_type = add_const_conditional_t<estd::is_const<Block>::value, page_noncv_type>;
    using pos_type = typename page_type::unit_type;

    using block_noncv_type = estd::remove_cv_t<Block>;

    template <class PoolTraits>
    friend class pool;

    template <class Traits>
    friend class pool_ops;

    static constexpr handle_type null = handles_traits::null;

    static_assert(Block::null == null);

    Block* block;
    page_type* page;
    handle_type handle{null};

    constexpr bundle_base() = default;

    // Conversion from non-const to const.  Kicks back if incoming Block2 is incompatible.
    // Tried enable_if_t here but errors are clearer without it.
    template <class Block2>
    constexpr bundle_base(const bundle_base<handles_traits, Block2>& convert_from) :
        block(convert_from.block),
        page(convert_from.page),
        handle(convert_from.handle)
    {}

    constexpr bundle_base(Block* block, page_type* page, handle_type handle) :
        block{block}, page{page}, handle{handle}
    {}

    constexpr modes mode() const { return block->mode_; }
    constexpr bool allocated() const { return block->allocated(); }

    constexpr pos_type pos() const { return page->pos(); }

    // DEBT: Consider renaming to not collide with handle null, especially since
    // at present block can be a super invalid pointer (0xFF location off in the weeds)
    constexpr bool is_null() const { return block == nullptr; }

    constexpr bool is_trivial() const { return block->mode() == v1::block_8::Trivial; }

    invariant_result invariant() const
    {
        if(is_null())
            EMBR_MEM_INVARIANT_ASSERT(page && handle == null, "Block is null but page and handle are not", "");

        // In fact, page CAN be null, we just rarely want it to be.  But it is a valid bundle technically
        //EMBR_MEM_INVARIANT_ASSERT(page != nullptr && page->is_null() == false,
        //    "Page cannot be null", "");

        // Despite null page validity, null handle is not.  Don't call get_bundle if your handle is null.
        // Only valid when entire bundle is nulled out
        EMBR_MEM_INVARIANT_ASSERT(handle != null || page != nullptr,
            "Handle cannot be null unless page is also null", "");

        if(page->is_null()) return {};

        return block->invariant();
    }

    void* data() const
    {
        return block->mode_ == RttoProxy ? block->proxy()->storage() : block->data();
    }

    constexpr bool has_prev() const { return block->prev() != null; }
    constexpr bool has_next() const { return block->next() != null; }

    constexpr bytes_unit<unsigned> header_size() const
    {
        // DEBT: Do a CTAD
        return bytes_unit<unsigned>{ Block::header_size(block->mode()) };
    }

    // DEBT: Sloppy way to convert between const and non-const bundles.
    template <class Block2>
    bundle_base& operator =(const bundle_base<handles_traits, Block2>& copy_from)
    {
        block = copy_from.block;
        page = copy_from.page;
        handle = copy_from.handle;
        return *this;
    }

private:
    // Bundle also serves as an accessor gateway, so that block can keep its data private otherwise
    // In turn, only 'pool' friend can access these

    void lock_up()              { ++block->lock_count_; }
    unsigned lock_down()        { return --block->lock_count_; }
    void lock_reset()           { block->lock_count_ = 0; }
    void ref_up() const         { ++block->ref_count_; }
    unsigned ref_down() const   { return --block->ref_count_; }

    void prev(handle_type v) const  { block->prev_ = v; }
    void next(handle_type v) const  { block->next_ = v; }
    void allocated(bool v) const    { block->allocated_ = v; }

    void mode(modes v)       { block->mode_ = v; }
    void reserve()                  { block->lock_count_ = 0xF; }

    using unconst_type = bundle_base<handles_traits, block_noncv_type>;

    // DEBT: Crude flavor of const_cast tuned just to our application.  Not TOO debt-y since it's for
    // internal use only
    constexpr unconst_type& unconst() const
    {
        return *(unconst_type*) this;
    }
};


}}

}}

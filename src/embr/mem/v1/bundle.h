#pragma once

#include "block.h"
#include "enum.h"
#include "fwd.h"
#include "page.h"
#include "traits.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class HandlesTraits, class Block, class Page>
struct bundle_base
{
    using handles_traits = HandlesTraits;
    using handle_type = typename HandlesTraits::size_type;
    using page_type = Page;
    using pos_type = typename page_type::unit_type;

    template <class PoolTraits>
    friend class pool;

    static constexpr handle_type null = handles_traits::null;

    Block* block;
    page_type* page;
    handle_type handle{null};

    constexpr bool allocated() const { return block->allocated(); }

    constexpr pos_type pos() const { return page->pos(); }

    // DEBT: Consider renaming to not collide with handle null
    constexpr bool is_null() const { return block == nullptr; }

    constexpr bool is_trivial() const { return block->mode() == v1::block::Trivial; }

    constexpr bool invariant() const
    {
        if(is_null())
            return page == nullptr && handle == null;

        return page != nullptr && page->is_null() == false &&
            handle != block_mode_base::null &&
            block->invariant();
    }

    void* data() const
    {
        return block->mode_ == block::RttoProxy ? block->proxy()->storage() : block->data();
    }

    constexpr bool has_prev() const { return block->prev() != null; }
    constexpr bool has_next() const { return block->next() != null; }

private:
    // Bundle also serves as an accessor gateway, so that block can keep its data private otherwise
    // In turn, only 'pool' friend can access these

    void lock_up()              { ++block->lock_count_; }
    unsigned lock_down()        { return --block->lock_count_; }
    void ref_up() const         { ++block->ref_count_; }
    unsigned ref_down() const   { return --block->ref_count_; }

    void prev(handle_type v) const  { block->prev_ = v; }
    void next(handle_type v) const  { block->next_ = v; }
    void allocated(bool v) const    { block->allocated_ = v; }

    void mode(block::modes v)       { block->mode_ = v; }
};

using bundle = bundle_base<handles_traits_base, v1::block, page<uint16_t>>;
using const_bundle = bundle_base<handles_traits_base, const v1::block, const page<uint16_t>>;


}}

}}

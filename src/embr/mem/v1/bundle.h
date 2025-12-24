#pragma once

#include "block.h"
#include "enum.h"
#include "fwd.h"
#include "page.h"
#include "traits.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class HandlesTraits, class Page = typename HandlesTraits::value_type>
struct bundle_base
{
    using handles_traits = HandlesTraits;
    using handle_type = typename HandlesTraits::size_type;
    using page_type = Page;
    using pos_type = typename page_type::unit_type;

    static constexpr handle_type null = handles_traits::null;

    v1::block* block;
    page_type* page;
    handle_type handle{null};

    constexpr pos_type pos() const { return page->pos(); }
    constexpr bool is_null() const { return block == nullptr; }

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

    // Bundle also serves as an accessor gateway, so that block can keep its data private otherwise

    void lock_up()              { ++block->lock_count_; }
    unsigned lock_down()        { return --block->lock_count_; }
    void ref_up() const         { ++block->ref_count_; }
    unsigned ref_down() const   { return --block->ref_count_; }

    void next(handle_type v) const  { block->next_ = v; }
};

using bundle = bundle_base<handles_traits_base, page<uint16_t>>;


}}

}}

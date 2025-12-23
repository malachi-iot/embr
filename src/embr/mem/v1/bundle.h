#pragma once

#include "block.h"
#include "enum.h"
#include "fwd.h"
#include "page.h"
#include "traits.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class HandlesTraits, class Page>
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

    constexpr bool has_prev() const { return block->prev() != null; }
    constexpr bool has_next() const { return block->next() != null; }
};

using bundle = bundle_base<handles_traits_base, page<uint16_t>>;


}}

}}

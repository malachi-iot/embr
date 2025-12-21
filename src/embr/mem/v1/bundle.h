#pragma once

#include "enum.h"
#include "fwd.h"
#include "page.h"
#include "traits.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class HandlesTraits, class Page>
struct bundle_base
{
    // DEBT: need wider-scope default page/handle type
    using handle_type = typename HandlesTraits::size_type;
    using page_type = Page;
    using pos_type = typename page_type::unit_type;

    v1::block* block;
    page_type* page;
    unsigned handle;

    constexpr pos_type pos() const { return page->pos(); }
    constexpr bool is_null() const { return block == nullptr; }

    constexpr bool invariant() const
    {
        if(is_null())   return true;

        return page != nullptr && page->is_null() == false && handle != block_mode_base::null;
    }
};

using bundle = bundle_base<handles_traits_base, page<uint16_t>>;


}}

}}

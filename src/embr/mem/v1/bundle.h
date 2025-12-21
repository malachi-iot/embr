#pragma once

#include "fwd.h"
#include "page.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

struct bundle
{
    // DEBT: need wider-scope default page/handle type
    using handle_type = uint8_t;
    using page_type = v1::page<uint16_t>;
    using pos_type = page_type::unit_type;

    v1::block* block;
    page_type* page;
    unsigned handle;

    constexpr pos_type pos() const { return page->pos(); }
    constexpr bool is_null() const { return block == nullptr; }

    constexpr bool invariant() const
    {
        if(is_null())   return true;

        return page != nullptr && page->is_null() == false && handle != block::null;
    }
};


}}

}}

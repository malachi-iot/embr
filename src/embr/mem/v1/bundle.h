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

    v1::block* block;
    page_type* page;
    unsigned handle;
};


}}

}}

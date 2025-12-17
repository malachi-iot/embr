#pragma once

#include "enum.h"
#include "fwd.h"
#include "page.h"


namespace embr { namespace mem {

namespace detail { inline namespace v1 {

class block : block_mode_base
{
    struct
    {
        unsigned prev_ : 8;
        unsigned next_ : 8;
        modes mode_ : 2;
    };

    char data_[];

public:
};


class small_block : block_mode_base
{
    struct
    {
        modes mode_ : 2;
    };
public:
};


struct bundle
{
    // DEBT: need wider-scope default page/handle type
    using handle_type = uint8_t;
    using page_type = v1::page<handle_type, estd::ratio<sizeof(void*)>>;

    v1::block* block;
    page_type* page;
    handle_type handle;
};

}}

}}

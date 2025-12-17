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

}}

}}

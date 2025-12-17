#pragma once

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

struct block_mode_base
{
    enum modes
    {
        Trivial,
        Rtto,
        RttoBase,
        Immobile
    };
};

}}

}}

#pragma once

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

struct block_mode_base
{
    enum modes
    {
        Trivial,
        RttoProxy,
        RttoBase,
        Immobile    // RttoProxy with no move constructor.  RttoBase not yet supported here
    };

    static constexpr unsigned null = 0xFF;
};

}}

}}

#pragma once

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

struct block_mode_enum
{
    enum modes
    {
        Trivial,
        RttoProxy,
        RttoBase,
        RttoVirtual,

        // EXPERIMENTAL
        // Trivial vector_control still occupies rtto space.  This might serve
        // to avoid that
        //Vector,

        // EXPERIMENTAL
        // System-wide RTTO avoider
        //UserType1,
    };

    static const char* to_string(modes);
};



inline const char* to_string(block_mode_enum::modes m)
{
    return block_mode_enum::to_string(m);
}

inline char to_abbrev(block_mode_enum::modes m);

}}

}}

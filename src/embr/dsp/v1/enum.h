#pragma once

#include <estd/flags.h>

namespace embr { namespace dsp { inline namespace v1 {

enum fixed_point_options
{
    FP_NONE             =       0,
    FP_UNSIGNED         =       0,
    FP_ENDIAN_NATIVE    =       0,
    FP_DEFAULT          =       FP_UNSIGNED | FP_ENDIAN_NATIVE,

    FP_SIGNED           =       0x01,
    FP_IMPLICIT         =       0x02,

    FP_ENDIAN_LE        =       0x10,
    FP_ENDIAN_BE        =       0x20
};


// NOTE: Don't pursue these strongly without considering embr::bits functionality overlap
enum packed_word_options
{
    PACKED_WORD_DEFAULT     =   0,              ///< msb, unswapped (native endianness)
    PACKED_WORD_NONE        =   0,
    PACKED_WORD_MSB         =   0x01,           ///< Hi order bit = msb
    PACKED_WORD_LSB         =   0x02,           ///< Hi order bit = lsb
    PACKED_WORD_SWAP        =   0x04,           ///< Consumer endianess - we merely swap on demand
    PACKED_WORD_SIGNED      =   0x08            ///< Implicitly treat ch0 as signed (whole value_type is signed)
};


ESTD_FLAGS(fixed_point_options)
ESTD_FLAGS(packed_word_options)

}}}


#pragma once

#include "../../../flags.h"

namespace embr { namespace v2 {

enum class word_options
{
    none,
    narrowing = 0x01,           ///< compile time enforcement to prohibit narrowing to less precise types
    init_masking = 0x02,
    storage_masking = 0x04,
    is_signed = 0x08,

    packed = 0x10,              ///< specify underlying storage as raw byte array.  Even-matched bounadries go to regular storage anyway
    raw = 0x20,                 ///< specify underlying storage as raw byte array - always  (DORMANT)

    implicit = 0x40,            ///< implicitly convert to integer

    //native = 0x1000,            ///< word internal contents is native endian
    big_endian = 0x2000,        ///< word internal contents is big endian
    little_endian = 0x4000,     ///< word internal contents is little endian
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    native = little_endian,
#else
    native = big_endian,
#endif
    endian_mask = big_endian | little_endian,

    masking = init_masking | storage_masking
};

EMBR_FLAGS(word_options)

}}

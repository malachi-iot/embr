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

ESTD_FLAGS(fixed_point_options)

}}}


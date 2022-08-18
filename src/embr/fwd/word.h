#pragma once

#include <estd/internal/platform.h>

#include "narrow_cast.h"

namespace embr {

// DEBT: Perhaps word_attributes might be better?
// DEBT: narrowing and arithmetic have opposite conventions - one is a disable, the other is an enable
enum class word_strictness
{
    none        = 0x00,
    narrowing   = 0x01,     ///< compile time enforcement to prohibit narrowing to less precise types
    masking     = 0x02,     ///< runtime enforcement to apply masks while assigning to word
    arithmetic  = 0x04,     ///< compile time permission to do integer arithmetic
    overflow    = 0x08,     ///< runtime indicator of overflow from shift operations - EXPERIMENTAL, not implemented

    narrowing_and_arithmetic = narrowing | arithmetic,
    narrowing_and_masking = narrowing | masking,
};

// Auto-sizing word
template <size_t bits, bool is_signed = false,
    word_strictness strict = word_strictness::narrowing_and_arithmetic>
class word;

}
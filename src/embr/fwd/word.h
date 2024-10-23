#pragma once

#include <estd/internal/platform.h>

#include "../internal/features.h"

#include "narrow_cast.h"
#include "type_from_bits.h"

namespace embr { inline namespace v1 {

#if FEATURE_EMBR_WORD_STRICTNESS
// DEBT: Perhaps word_attributes might be better?
// DEBT: narrowing and arithmetic have opposite conventions - one is a disable, the other is an enable
enum class word_strictness
{
    none            = 0x00,
    narrowing       = 0x01,     ///< compile time enforcement to prohibit narrowing to less precise types
    init_masking    = 0x02,     ///< runtime enforcement to apply masks while assigning to word
    arithmetic      = 0x04,     ///< compile time permission to do integer arithmetic
    overflow        = 0x08,     ///< runtime indicator of overflow from shift operations - EXPERIMENTAL, not implemented
    initialized     = 0x10,     ///< when true, disables default constructor thus requiring initialization - not implemented
    storage_masking = 0x20,     ///< runtime enforcement to apply masks while retrieving underlying word (useful for unions)
    overflow2       = 0x40,     ///< when true, observes word-limit::max() as rollover point - EXPERIMENTAL, not implemented
                                
    masking = init_masking | storage_masking,
    narrowing_and_arithmetic = narrowing | arithmetic,
    narrowing_and_masking = narrowing | masking,
};
#else
// DEBT: A kind of fake monostate.  Monostate is available here, but can't be instantiated
// as a default template value parameter in c++03, so we use unsigned
typedef unsigned word_strictness;
#endif

// Auto-sizing word
template <size_t bits, bool is_signed = false, word_strictness strict =
#if FEATURE_EMBR_WORD_STRICTNESS
    word_strictness::narrowing_and_arithmetic,
#else
    word_strictness(),
#endif
    typename TInt = typename type_from_bits<bits, is_signed>::type>
class word;

}}

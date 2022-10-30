#pragma once

#include <estd/internal/platform.h>

// DEBT: Move this out to a feature.h area
#ifdef FEATURE_CPP_ENUM_CLASS
#ifndef FEATURE_EMBR_WORD_STRICTNESS
#define FEATURE_EMBR_WORD_STRICTNESS 1
#endif
#endif

#include "narrow_cast.h"

namespace embr {

#if FEATURE_EMBR_WORD_STRICTNESS
// DEBT: Perhaps word_attributes might be better?
// DEBT: narrowing and arithmetic have opposite conventions - one is a disable, the other is an enable
enum class word_strictness
{
    none        = 0x00,
    narrowing   = 0x01,     ///< compile time enforcement to prohibit narrowing to less precise types
    masking     = 0x02,     ///< runtime enforcement to apply masks while assigning to word
    arithmetic  = 0x04,     ///< compile time permission to do integer arithmetic
    overflow    = 0x08,     ///< runtime indicator of overflow from shift operations - EXPERIMENTAL, not implemented
    initialized = 0x10,     ///< when true, disables default constructor thus requiring initialization - not implemented

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
    word_strictness::narrowing_and_arithmetic
#else
    word_strictness()
#endif
    >
class word;

// NOTE: This macro is useful so that c++11 can have a trivial constructor, but still
// be c++03 compatible
// DEBT: Consider putting this into ESTD
#if __cpp_constexpr
#define EMBR_CPP_DEFAULT_CTOR(name) constexpr name() = default;
#else
#define EMBR_CPP_DEFAULT_CTOR(name) inline name() {};
#endif

}
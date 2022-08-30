#pragma once

// This include is primarily for access to ESTD_CPP_CONSTEXPR_RET
#include <estd/internal/locale.h>

#include "../fwd/word.h"

#if FEATURE_EMBR_WORD_STRICTNESS
#include "../enum_mask.h"   // NOTE: Only in support of experimental 'attributes'
#endif

#include "narrow_cast.h"
#include "type_from_bits.h"

namespace embr {

#if FEATURE_EMBR_WORD_STRICTNESS

// DEBT: These are located here because internal::word_base needs them.
// perhaps a "word_strictness.h" is appropriate?

template <word_strictness s>
using strictness_helper = internal::enum_mask<word_strictness, s>;

/// Determines if any 's' bits are present in 'v'
template <word_strictness v, word_strictness... s>
constexpr bool any()
{
    return strictness_helper<v>::template any<s...>();
}

/// Determines if all 's' bits are present in 'v'
template <word_strictness v, word_strictness... s>
constexpr bool all()
{
    return strictness_helper<v>::template all<s...>();
}

#endif


namespace internal {

// This is the opposite of what we want.  We actually want narrow_cast to be very permissive.
// to_integer already does compile time disallow of too much narrowing
/*
template <size_t bits_cast_from, size_t bits_cast_to>
struct narrow_cast<word<bits_cast_from>, word<bits_cast_to>,
    typename estd::enable_if<(bits_cast_from <= bits_cast_to)>::type>
{

}; */


#if __cplusplus >= 201103L
template <size_t bits_cast_from, bool signed_from, size_t bits_cast_to, bool signed_to, word_strictness strict>
struct narrow_cast<word<bits_cast_from, signed_from>, word<bits_cast_to, signed_to, strict> >
{
    typedef word<bits_cast_to, signed_to, strict> to_type;

    static constexpr to_type cast(word<bits_cast_from, signed_from>&& from)
    {
        return to_type{
            (typename to_type::type)from.cvalue()};
    }
};
#endif

// Analogous to std::byte, but with choosable bit count

// A separate word_base is out here in internal so that:
// a) convenient place to experiment with mask & attributes
// b) we can consolidate the current c++03 incompatible (any/all) mechanisms
// c) nice to have the ceremony of all the operator overloads elsewhere (in 'word')
template <size_t bits, bool is_signed, word_strictness strict>
class word_base : public type_from_bits<bits, is_signed>
{
    typedef type_from_bits<bits, is_signed> base_type;
#if FEATURE_EMBR_WORD_STRICTNESS
    typedef internal::enum_mask<word_strictness, strict> h;
#endif

    // Equivalent to  ((1 << bits) - 1) without overflowing
    static CONSTEXPR std::uintmax_t mask_ =
        (((std::uintmax_t)1 << (bits - 1)) - 1) | ((std::uintmax_t)1 << (bits - 1));

public:
    typedef typename base_type::type type;

protected:

    type value_;

#if FEATURE_EMBR_WORD_STRICTNESS
    constexpr word_base(const type& value) : value_
        {any<strict, word_strictness::masking>() ? mask(value) : value} {}
    constexpr word_base(type&& value) : value_{
        any<strict, h::e::masking>() ? mask(value) : value} {}
#else
    ESTD_CPP_CONSTEXPR_RET word_base(const type& value) : value_(value) {}
#ifdef FEATURE_CPP_MOVESEMANTIC
    constexpr word_base(type&& value) : value_{value} {}
#endif
#endif

public:
    ESTD_CPP_CONSTEXPR_RET const type& value() const { return value_; }
    ESTD_CPP_CONSTEXPR_RET const type& cvalue() const { return value_; }

#if FEATURE_EMBR_WORD_STRICTNESS
    // EXPERIMENTAL
    typedef internal::enum_mask<word_strictness, strict> attributes;
#endif

    static ESTD_CPP_CONSTEXPR_RET unsigned width() { return bits; }

    // DEBT: Pretty sure we have to adjust this for signed operations
    static ESTD_CPP_CONSTEXPR_RET type mask()
    {
        return mask_;
    }
    
    static ESTD_CPP_CONSTEXPR_RET type mask(type v)
    {
        return v & mask();
    }
};



}


}
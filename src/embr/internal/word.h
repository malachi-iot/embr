#pragma once

#include "../fwd/word.h"

#include "../enum_mask.h"   // NOTE: Only in support of experimental 'attributes'

#include "narrow_cast.h"
#include "type_from_bits.h"

namespace embr {
    
namespace internal {

// This is the opposite of what we want.  We actually want narrow_cast to be very permissive.
// to_integer already does compile time disallow of too much narrowing
/*
template <size_t bits_cast_from, size_t bits_cast_to>
struct narrow_cast<word<bits_cast_from>, word<bits_cast_to>,
    typename estd::enable_if<(bits_cast_from <= bits_cast_to)>::type>
{

}; */


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


// Analogous to std::byte, but with choosable bit count

template <size_t bits, bool is_signed, word_strictness strict>
class word_base : public type_from_bits<bits, is_signed>
{
    // Equivalent to  ((1 << bits) - 1) without overflowing
    static constexpr std::uintmax_t mask_ =  
        (((std::uintmax_t)1 << (bits - 1)) - 1) | ((std::uintmax_t)1 << (bits - 1));

public:
    typedef typename type_from_bits<bits, is_signed>::type type;

    // EXPERIMENTAL
    typedef internal::enum_mask<word_strictness, strict> attributes;

    static constexpr unsigned width() { return bits; }

    // DEBT: Pretty sure we have to adjust this for signed operations
    static constexpr type mask()
    {
        return mask_;
    }
    
    static constexpr type mask(type v)
    {
        return v & mask();
    }
};



}


}
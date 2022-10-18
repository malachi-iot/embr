#pragma once

#include <estd/cstdint.h>

#include <estd/array.h>

#include "decoder.h"
#include "encoder.h"

#include "../platform/guard-in.h"

namespace embr { namespace bits {

#ifdef __cpp_alias_templates

namespace internal {

/// Utility class combining encoder and decoder together.  Name still in flux
/// @tparam e
/// @tparam direction
/// TODO: Likely this wants to be formalized into 'detail' namespace
template <endianness e, length_direction direction, resume_direction rd, class TBase>
using material = decoder<e, direction, rd, encoder<e, direction, rd,
    embr::bits::internal::provider<e, TBase> > >;

}



namespace layer1 {

template <endianness e, size_t N, length_direction direction = default_direction>
using material = embr::bits::internal::material<e, direction, direction, estd::array<uint8_t, N> >;

}


// These *imply* an upper boundary, though it is not enforced, except on == overload.
// Since we derive from array, you get access to size() etc
namespace layer2 {

template <endianness e, size_t N,
    length_direction direction = default_direction,
    resume_direction rd = direction
    >
using material = embr::bits::internal::material<e, direction, rd, estd::layer2::array<uint8_t, N> >;


template <endianness e, size_t N, length_direction direction = default_direction, resume_direction rd = direction>
using decoder = embr::bits::decoder<e, direction, rd,
    internal::provider<e, estd::layer2::array<uint8_t, N> > >;

}

#endif


namespace experimental {

// NOTE: Unsure if I like having direction here, but seems useful
template <unsigned bitpos_, unsigned length_, length_direction direction_ = default_direction>
struct bit_traits
{
    static CONSTEXPR unsigned bitpos = bitpos_;
    static CONSTEXPR unsigned length = length_;
    static CONSTEXPR length_direction direction = direction_;

    typedef word<length> word_type;

    // A little clumsy, remember we need an indicator of the full containing word bit size
    // where above word_type is the subset word.  So we add length_ and bitpos_ together,
    // which will nudge bit count high enough for descriptor_base to specialize and choose
    // the bitness_type big enough to hold on to the desired values
    typedef internal::descriptor_base<length_ + bitpos_> descriptor_type;
    typedef typename descriptor_type::uint_type descriptor_uint_type;

    static ESTD_CPP_CONSTEXPR_RET descriptor_type get_descriptor()
    {
        return descriptor_type(bitpos, length);
    }

    static ESTD_CPP_CONSTEXPR_RET descriptor_uint_type mask() { return (1 << length) - 1; }
    static ESTD_CPP_CONSTEXPR_RET descriptor_uint_type shifted_mask() { return mask() << bitpos; }

#ifdef FEATURE_CPP_DEFAULT_TARGS
    template <class T = descriptor_uint_type, length_direction d = direction_>
#else
    template <class T, length_direction d>
#endif
    static ESTD_CPP_CONSTEXPR_RET word_type get(const T* raw)
    {
        // FIX: msb_to_lsb mode still needed here
        return d == lsb_to_msb ? ((*raw >> bitpos) & mask()) : 0;
    }

#ifdef FEATURE_CPP_DEFAULT_TARGS
    template <class T = descriptor_uint_type, length_direction d = direction_>
#else
    template <class T, length_direction d>
#endif
    static ESTD_CPP_CONSTEXPR_RET descriptor_uint_type set(T* raw, word_type value)
    {
        return d == lsb_to_msb ?
            (*raw &= ~shifted_mask()) |= estd::to_integer<descriptor_uint_type>(value) << bitpos :
            0;
    }

};

}

}}


#define EMBR_BITS_MATERIAL_PROPERTY(name, offset, bitpos, length) \
    EMBR_BITS_DECODER_GETTER(name, offset, bitpos, length)        \
    EMBR_BITS_ENCODER_SETTER(name, offset, bitpos, length)

// DEBT: Under unknown conditions, esp-idf GCC gets upset about uninitialized
// variables passed in by reference.  I too have my misgivings about that pattern,
// but it's a valid one.  This flag force-initializes variables in question,
// thus inhibiting the promoted warning->error.
// ---
// Specifically, esp-idf GCC defaults to promoting warnings to errors.  Works for me.
// Then, it notices that sometimes a variable passed somewhere by reference hasn't
// been initialized yet, some it warns us "maybe-uninitialized".  So far so good.
// This results in an error.  So, under those circumstances, it would make sense to
// pragma GCC disable that warning.  That's part 1 of where things get odd.
// The pragmas seem to do nothing in this instance.
// Then, there's part 2 - the unexplained behavior that the seemingly same compiler
// configuration can compile embr::bits code base just fine under our esp-idf
// unity tests, but under chariot project we experience the warning.
#ifndef WORKAROUND_EMBR_MAYBE_UNINITIALIZED
#define WORKAROUND_EMBR_MAYBE_UNINITIALIZED 0
#endif

#include "../platform/guard-out.h"

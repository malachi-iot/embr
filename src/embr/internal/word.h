#pragma once

#include <estd/algorithm.h>
#include <estd/bit.h>
// This include is primarily for access to ESTD_CPP_CONSTEXPR_RET
#include <estd/internal/locale.h>

#include "../fwd/word.h"

#if FEATURE_EMBR_WORD_STRICTNESS
#include "../enum_mask.h"   // NOTE: Only in support of experimental 'attributes'
#endif

#include "flags.h"
#include "narrow_cast.h"
#include "type_from_bits.h"

namespace embr { inline namespace v1 {

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

}}  // embr::v1

namespace embr { namespace internal {

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
template <size_t bits, bool is_signed, word_strictness strict,
    typename T = typename type_from_bits<bits, is_signed>::type >
class word_base
{
#if FEATURE_EMBR_WORD_STRICTNESS
    typedef internal::enum_mask<word_strictness, strict> h;
#endif

    // Equivalent to  ((1 << bits) - 1) without overflowing
    static CONSTEXPR std::uintmax_t mask_ =
        (((std::uintmax_t)1 << (bits - 1)) - 1) | ((std::uintmax_t)1 << (bits - 1));

public:
    typedef T type;

protected:

    type value_;

    ESTD_CPP_DEFAULT_CTOR(word_base)

#if FEATURE_EMBR_WORD_STRICTNESS
    static constexpr bool do_init_masking() { return any<strict, word_strictness::init_masking>(); }

    explicit constexpr word_base(const type& value) : value_
        {do_init_masking() ? mask(value) : value} {}
    explicit constexpr word_base(type&& value) : value_
        {do_init_masking() ? mask(value) : value} {}
#else
    ESTD_CPP_CONSTEXPR_RET word_base(const type& value) : value_(value) {}
#ifdef FEATURE_CPP_MOVESEMANTIC
    constexpr word_base(type&& value) : value_{value} {}
#endif
#endif

public:
    ESTD_CPP_CONSTEXPR_RET type cvalue() const
    {
#if FEATURE_EMBR_WORD_STRICTNESS
        return any<strict, word_strictness::storage_masking>() ? mask(value_) : value_;
#else
        return value_;
#endif
    }

    ESTD_CPP_CONSTEXPR_RET type value() const { return cvalue(); }

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

template <v2::word_options o>
using map_to_endian = estd::integral_constant<
    estd::endian,
    !(o & v2::word_options::endian_mask) ? estd::endian::native :
        o & v2::word_options::big_endian ? estd::endian::big : estd::endian::little>;


template <v2::word_options o>
using is_native_endian = estd::bool_constant<
    (!(o & v2::word_options::endian_mask) ||
    ((o & v2::word_options::endian_mask) == v2::word_options::native))>;


// FIX: This guy is starting to head in too many directions at once
template <v2::word_options o, class enabled = void>
struct word_retriever;

template <v2::word_options o>
struct word_retriever<o, estd::enable_if_t<
    !(o & v2::word_options::packed) &&
    is_native_endian<o>::value>>
{
    template <class Numeric>
    static constexpr Numeric get(const Numeric& v) { return v; }
};

template <v2::word_options o>
struct word_retriever<o, estd::enable_if_t<
    o & v2::word_options::packed &&
    is_native_endian<o>::value == true>>
{
    template <class Numeric>
    static constexpr Numeric get(const Numeric& v)
    {
        return estd::byteswap(v);
    }

    static void get(const uint8_t* in, uint32_t* out)
    {
        // I think we could cast out to uint8_t* and initialize it directly too
        uint8_t v[4] { 0, in[2], in[1], in[0] };
        *out = *(uint32_t*) v;
    }
};


template <class T, size_t size, estd::endian target,
    estd::endian platform = estd::endian::native>
struct packer;

// NOTE: This flavor truncates on unpack
template <>
struct packer<uint16_t, 3, estd::endian::little, estd::endian::little>
{
    static void pack(uint16_t in, uint8_t* out)
    {
        out[0] = 0;
        out[1] = in >> 8;
        out[2] = in & 0xFF;
    }

    static uint16_t unpack(const uint8_t* in)
    {
        uint16_t v = in[1];
        v <<= 8;
        v |= in[0];
        return v;
    }
};

// NOTE: This flavor truncates on pack
template <>
struct packer<uint32_t, 3, estd::endian::little, estd::endian::little>
{
    static uint8_t* pack(uint32_t in, uint8_t* out)
    {
        out[0] = in >> 16 & 0xFF;
        out[1] = in >> 8 & 0xFF;
        out[2] = in & 0xFF;
        return out;
    }

    static constexpr uint32_t unpack(const uint8_t* in)
    {
        return in[0] << 16 | in[1] << 8 | in[2];
    }
};

// NOTE: This flavor truncates on unpack
template <>
struct packer<uint32_t, 6, estd::endian::little, estd::endian::little>
{
    static uint8_t* pack(uint32_t in, uint8_t* out)
    {
        *((uint32_t*) out) = in;
        out[4] = 0;
        out[5] = 0;
        /*
        out[2] = in >> 24;
        out[3] = in >> 16;
        out[4] = in >> 8;
        out[5] = in;    */
        return out;
    }

    static constexpr uint32_t unpack(const uint8_t* in)
    {
        return * (const uint32_t*) in + 2;
        //return in[2] << 24 | in[3] << 16 | in[4] << 8 | in[5];
    }
};


// NOTE: This flavor truncates on on pack
template <>
struct packer<uint64_t, 6, estd::endian::little, estd::endian::little>
{
    static uint8_t* pack(uint64_t in, uint8_t* out)
    {
        /*
        out[0] = in >> 40;
        out[1] = in >> 32;
        out[2] = in >> 24;
        out[3] = in >> 16;
        out[4] = in >> 8;
        out[5] = in;*/
        // native flavor, we can do pure byte copies and pointer tricks
        estd::copy_n(((uint8_t*)&in), 6, out);
        return out;
    }

    static uint64_t unpack(const uint8_t* in)
    {
        uint64_t out;

        estd::copy_n(in, 6, (uint8_t*)&out);
        return out;
    }

    static constexpr uint64_t unpack_constexpr(const uint8_t* in)
    {
        // Could do this with copy_n in theory, but that would require some fancy casting
        // and trickier to make constexpr
        return uint64_t(in[5]) << 40 | uint64_t(in[4]) << 32 | in[3] << 24 | in[2] << 16 | in[1] << 8 | in[0];
    }
};


// native endian flavor using regular storage
template <size_t bits, v2::word_options o>
//struct word_v2_base<bits, o, estd::enable_if_t<o == v2::word_options::native>>
struct word_v2_base<bits, o,
    estd::enable_if_t<
        is_native_endian<o>::value &&
        !(o & v2::word_options::packed) ||
        type_from_bits<bits, false>::matched>> :
    type_from_bits<bits, o & v2::word_options::is_signed>
{
    using base_type = type_from_bits<bits, o & v2::word_options::is_signed>;
    using typename base_type::type;

    static constexpr estd::endian endian = estd::endian::native;

    type v_;

    word_v2_base() = default;
    constexpr word_v2_base(const type& copy_from) : v_{copy_from}
    {
    }

    constexpr operator type() const { return v_; }
};


// non-native endian flavor using regular storage
template <size_t bits, v2::word_options o>
struct word_v2_base<bits, o,
    estd::enable_if_t<
        is_native_endian<o>::value == false &&
            !(o & v2::word_options::packed) ||
        type_from_bits<bits, false>::matched>> :
    type_from_bits<bits, o & v2::word_options::is_signed>
{
    using base_type = type_from_bits<bits, o & v2::word_options::is_signed>;
    using typename base_type::type;

    static constexpr estd::endian endian = map_to_endian<o>::value;

    // Stored in "wrong" order (byte swapped)
    type v_;

    word_v2_base() = default;
    constexpr word_v2_base(const type& copy_from) :
        v_{estd::byteswap(copy_from)}
    {
    }

    constexpr operator type() const
    {
        return estd::byteswap(v_);
    }
};


// native flavor using raw byte storage
template <size_t bits, v2::word_options o>
struct word_v2_base<bits, o,
    estd::enable_if_t<
        is_native_endian<o>::value &&
        o & v2::word_options::packed &&
        type_from_bits<bits, false>::matched == false>> :
        //true>> :
    type_from_bits<bits, o & v2::word_options::is_signed>
{
    using base_type = type_from_bits<bits, o & v2::word_options::is_signed>;
    using typename base_type::type;

    static constexpr estd::endian endian = estd::endian::native;

    using pack = packer<type, base_type::size, endian>;

    uint8_t raw_[base_type::size];

    word_v2_base() = default;
    word_v2_base(const type& copy_from)
    {
        pack::pack(copy_from, raw_);
    }

    constexpr operator type() const
    {
        return pack::unpack(raw_);
    }
};


}}  // embr::internal

namespace embr { namespace v2 {

template <size_t bits, word_options o>
struct word : internal::word_v2_base<bits, o>
{
    using base_type = internal::word_v2_base<bits, o>;
    using typename base_type::type;

public:
    word() = default;
    constexpr word(const type& copy_from) : base_type(copy_from)
    {
    }
};

}}

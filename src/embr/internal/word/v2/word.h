#pragma once

#include "../packer.h"

#include "enum.h"
#include "fwd.h"

namespace embr { namespace internal {

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
struct word_retriever<o,
    estd::enable_if_t<
        !(o & v2::word_options::packed) &&
        is_native_endian<o>::value>>
{
    template <class Numeric>
    static constexpr Numeric get(const Numeric& v) { return v; }
};

template <v2::word_options o>
struct word_retriever<o,
    estd::enable_if_t<
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


// Helper function to access the nth element of an array at compile time.
template <size_t N, typename T, size_t Size>
constexpr T get_element(const T (&arr)[Size])
{
    static_assert(N < Size, "Index out of bounds");
    return arr[N];
}


// native endian flavor using regular storage
template <size_t bits, v2::word_options o>
//struct word_v2_base<bits, o, estd::enable_if_t<o == v2::word_options::native>>
struct word_v2_base<bits, o,
    estd::enable_if_t<
        is_native_endian<o>::value &&
        (!(o & v2::word_options::packed) ||
            type_from_bits<bits, false>::matched)>> :
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
        (!(o & v2::word_options::packed) ||
            type_from_bits<bits, false>::matched)>> :
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

    constexpr bool operator==(const word_v2_base& compare_to) const
    {
        return v_ == compare_to.v;
    }
};


// flavor using raw byte storage
template <size_t bits, v2::word_options o>
struct word_v2_base<bits, o,
    estd::enable_if_t<
        //is_native_endian<o>::value &&
        o & v2::word_options::packed &&
        type_from_bits<bits, false>::matched == false>> :
    type_from_bits<bits, o & v2::word_options::is_signed>
{
    using base_type = type_from_bits<bits, o & v2::word_options::is_signed>;
    using typename base_type::type;
    using base_type::size;

    static constexpr estd::endian endian = map_to_endian<o>::value;

    using pack = packer<type, base_type::size, endian>;

    uint8_t raw_[base_type::size];

    word_v2_base() = default;
    word_v2_base(const type& copy_from)
    {
        pack::pack(copy_from, raw_);
    }

    /*
#if __cpp_initializer_lists
    constexpr word_v2_base(std::initializer_list<uint8_t> copy_from) :
        raw_{copy_from}
    {}
#endif  */
    //template <typename... Args>
    //constexpr word_v2_base(float, Args&&...args) : raw_{ std::forward<Args>(args)... } {}

private:
    template <size_t... I>
    explicit constexpr word_v2_base(
        const uint8_t (&raw)[size],
        estd::index_sequence<I...>) :
        raw_{get_element<I>(raw)...}
    {

    }

public:
    constexpr word_v2_base(const uint8_t (&raw)[size]) :
        word_v2_base(raw, estd::make_index_sequence<size>{})
    {

    }

    // This actually works, but I am annoyed by the double-init of raw
#if UNUSED
    constexpr word_v2_base(const uint8_t (&raw)[base_type::size])
        // DEBT: gcc 12.2 needs this initializer-list init, others didn't
        : raw_{}
    {
        // surprisingly this is c++11 constexpr-friendly
        for(int i = 0; i < base_type::size; i++) raw_[i] = raw[i];
    }
#endif


    constexpr operator type() const
    {
        return pack::unpack(raw_);
    }

    constexpr bool operator==(const word_v2_base& compare_to) const
    {
        return estd::equal(raw_, raw_ + base_type::size, compare_to.raw_);
    }
};

/*
// non-native flavor using raw byte storage
template <size_t bits, v2::word_options o>
struct word_v2_base<bits, o,
    estd::enable_if_t<
        is_native_endian<o>::value == false &&
        o & v2::word_options::packed &&
        type_from_bits<bits, false>::matched == false>> :
    type_from_bits<bits, o & v2::word_options::is_signed>
{
};
*/

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

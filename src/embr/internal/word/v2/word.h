#pragma once

#include "../packer.h"

#include "../../type_from_bits.h"
#include "common_type.h"
#include "enum.h"
#include "fwd.h"
#include "numeric_limits.h"

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

template <v2::word_options o1, v2::word_options o2>
using is_matching_endian = estd::bool_constant<map_to_endian<o1>::value == map_to_endian<o2>::value>;


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
    using this_type = word_v2_base;

    using typename base_type::type;

    static constexpr estd::endian endian = estd::endian::native;

    type v_;

    word_v2_base() = default;
    constexpr word_v2_base(const type& copy_from) : v_{copy_from}
    {
    }

    constexpr type value() const { return v_; }

    // NOTE: Somehow bool operator==() doesn't get picked up here.  Just as well,
    // I prefer external == anyway.  Perhaps that's the c++ language expectation?
    template <size_t bits2, v2::word_options o2, class Enabled = void>
    constexpr bool equals(const word_v2_base<bits2, o2>& compare_to) const
    {
        return v_ == compare_to.value();
    }

    this_type& operator++()
    {
        ++v_;
        return *this;
    }

    this_type& operator+=(const type& v)
    {
        v_ += v;
        return *this;
    }
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

    constexpr type value() const { return estd::byteswap(v_); }

    /* almost there, just conflicts with other equals
    template <size_t bits2, v2::word_options o2,
        class Enabled = estd::enable_if_t<
            is_matching_endian<o, o2>::value &&
            estd::is_same<typename word_v2_base<bits2, o2>::type, type>::value
        >>
    constexpr bool equals(const word_v2_base<bits2, o2>& compare_to) const
    {
        return v_ == compare_to.v_;
    }   */

    template <size_t bits2, v2::word_options o2>
    constexpr bool equals(const word_v2_base<bits2, o2>& compare_to, bool = {}) const
    {
        return value() == compare_to.value();
    }

    constexpr bool operator==(const word_v2_base& compare_to) const
    {
        return v_ == compare_to.v_;
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
    using this_type = word_v2_base;

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


    constexpr type value() const
    {
        return pack::unpack(raw_);
    }

    this_type& operator+=(const type& addendum)
    {
        type added = value() + addendum;
        pack::pack(added, raw_);
        return *this;
    }

    template <size_t bits2, v2::word_options o2>
    this_type& operator+=(const word_v2_base<bits2, o2>& addendum)
    {
        return operator+=(addendum.value());
    }

    constexpr bool operator==(const word_v2_base& compare_to) const
    {
        return estd::equal(raw_, raw_ + base_type::size, compare_to.raw_);
    }

    template <size_t bits2, v2::word_options o2>
    constexpr bool equals(const word_v2_base<bits2, o2>& compare_to) const
    {
        return value() == compare_to.value();
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

template <size_t bits, v2::word_options o>
struct word_v2_layer<bits, o, estd::enable_if_t<o & v2::word_options::implicit>> :
    word_v2_base<bits, o>
{
    using base_type = word_v2_base<bits, o>;
    using typename base_type::type;

    ESTD_CPP_FORWARDING_CTOR(word_v2_layer)

    // DEBT: I think we can achieve this by enable_if on return type
    constexpr operator type() const
    {
        return base_type::value();
    }
};

template <size_t bits, v2::word_options o>
struct word_v2_layer<bits, o, estd::enable_if_t<!(o & v2::word_options::implicit)>> :
    word_v2_base<bits, o>
{
    using base_type = word_v2_base<bits, o>;
    using typename base_type::type;

    ESTD_CPP_FORWARDING_CTOR(word_v2_layer)
};


}}  // embr::internal


namespace embr { namespace v2 {

#if !FEATURE_EMBR_WORD_ALIAS
template <size_t bits, word_options o, uint16_t padding>
struct word : internal::word_v2_layer<bits, o>
{
    using base_type = internal::word_v2_layer<bits, o>;
    using typename base_type::type;

public:
    // Doesn't pick up implicit =
    //ESTD_CPP_FORWARDING_CTOR(word)

    word() = default;
    constexpr word(const type& copy_from) : base_type(copy_from)
    {
    }

    template <size_t bits2, word_options o2>
    constexpr word(const internal::word_v2_base<bits2, o>& copy_from) :
        base_type(copy_from)
    {

    }
};
#endif

}}

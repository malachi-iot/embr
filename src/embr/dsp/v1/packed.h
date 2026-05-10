#pragma once

#include "fwd.h"

#include <embr/internal/type_from_bits.h>
#include <estd/flags.h>

// DEBT: Strong overlap with embr bits, word, material areas.  Consolidate.

namespace embr { namespace dsp {

// channel order starts with msb. i.e. 16-bit uint
// high order 4 bits would be channel 0, next 12 bits would be channel 1
//template <unsigned ...channel_bits>
//struct packed_word;

namespace detail { inline namespace v1 {

template <packed_word_options o, unsigned ch0_bits, unsigned ch1_bits>
class packed_word_base<
    o,
    estd::integer_sequence<unsigned, ch0_bits, ch1_bits>,
    // DEBT: Doesn't work against 'false', but should
    estd::enable_if_t<(o & PACKED_WORD_SWAP) == PACKED_WORD_NONE>>
{
public:
    static constexpr unsigned total_bits = ch0_bits + ch1_bits;
    static constexpr bool is_signed = o & PACKED_WORD_SIGNED;
    using value_type = type_from_bits_t<total_bits, is_signed>;
    using unsigned_type = type_from_bits_t<total_bits, false>;
    using signed_type = type_from_bits_t<total_bits, true>;

protected:
    static constexpr unsigned ch1_mask = (1 << ch1_bits) - 1;
    static constexpr unsigned ch0_pos = total_bits - ch0_bits;

public:
    // DEBT: Make this protected again - needed by unit tests
    value_type v_;

    packed_word_base() = default;

    explicit constexpr packed_word_base(value_type v) : v_{v}   {}

    constexpr packed_word_base(value_type ch0, value_type ch1) :
        v_{value_type(ch0 << ch0_pos | ch1)}
    {}

    constexpr value_type value() const { return v_; }

    template <unsigned Index, class T = value_type>
    constexpr T channel() const
    {
        static_assert(Index == 0 || Index == 1, "Only 2 channels supported for this type");

        if constexpr(Index == 0)
        {
            return static_cast<T>(v_) >> ch0_pos;
        }
        else
        {
            return static_cast<T>(v_) & ch1_mask;
        }
    }
};


// UNTESTED
template <packed_word_options o, unsigned ch0_bits, unsigned ch1_bits>
class packed_word_base<
    o,
    estd::integer_sequence<unsigned, ch0_bits, ch1_bits>,
    estd::enable_if_t<
        (o == PACKED_WORD_SWAP) &&
        (ch0_bits + ch1_bits) == 16>>
{
    using value_type = uint16_t;

    uint16_t v_;

    static constexpr unsigned total_bits = ch0_bits + ch1_bits;
    static constexpr unsigned ch0_pos = total_bits - ch0_bits;

    static constexpr unsigned ch1_mask = (1 << ch1_bits) - 1;

public:
    template <unsigned Index>
    constexpr value_type channel() const
    {
        static_assert(Index <= 1, "Only 2 channels supported for this type");

        if constexpr(ch1_bits <= 8 && Index == 1)
        {
            return v_ >> 8 & ch1_mask;
        }
        else if constexpr(Index == 0)
        {
            return estd::byteswap(v_) >> ch0_pos;
        }
        else
        {
            return estd::byteswap(v_) & ch1_mask;
        }
    }
};

// DEBT: Go full variadic, if we can
template <packed_word_options o, unsigned ch0_bits, unsigned ch1_bits, unsigned ch2_bits>
class packed_word_base<
    o,
    estd::integer_sequence<unsigned, ch0_bits, ch1_bits, ch2_bits>,
    estd::enable_if_t<o == PACKED_WORD_DEFAULT>>
{
    static constexpr unsigned total_bits = ch0_bits + ch1_bits + ch2_bits;
    static constexpr unsigned ch0_pos = total_bits - ch0_bits;
    static constexpr unsigned ch1_pos = ch0_pos - ch1_bits;
    static constexpr unsigned ch1_mask = (1 << ch1_bits) - 1;
    static constexpr unsigned ch2_mask = (1 << ch2_bits) - 1;
    //static constexpr unsigned ch3_pos = 0;

public:
    using value_type = type_from_bits_t<total_bits, false>;

    template <unsigned Index>
    using index = estd::integral_constant<unsigned, Index>;

protected:
    value_type v_;

public:
    packed_word_base() = default;

    // DEBT: No upper masking
    constexpr packed_word_base(unsigned ch0, unsigned ch1, unsigned ch2) :
        v_{static_cast<value_type>(ch0 << ch0_pos | ch1 << ch1_pos | ch2)}
    {}

    template <unsigned Index>
    constexpr value_type channel() const
    {
        static_assert(Index <= 2, "Only 3 channels supported for this type");

        if constexpr(Index == 0)
        {
            return v_ >> ch0_pos;
        }
        else if constexpr(Index == 1)
        {
            return (v_ >> ch1_pos) & ch1_mask;
        }
        else
        {
            return v_ & ch2_mask;
        }
    }

    constexpr value_type value() const { return v_; }
};


template <packed_word_options o, unsigned ch0_bits, unsigned ch1_bits, unsigned ch2_bits>
class packed_word_base<
    o,
    estd::integer_sequence<unsigned, ch0_bits, ch1_bits, ch2_bits>,
    estd::enable_if_t<
        o == PACKED_WORD_SWAP &&
        (ch0_bits < 8) && (ch1_bits <= 8) &&
        (ch0_bits + ch1_bits + ch2_bits) == 16>>
{
    // Always 16
    static constexpr unsigned total_bits = ch0_bits + ch1_bits + ch2_bits;
    static constexpr unsigned ch0_pos = 8 - ch0_bits;
    static constexpr unsigned ch1_pos = ch2_bits;
    static constexpr unsigned ch1_bits_a = ch0_pos;
    static constexpr unsigned ch1_bits_b = ch1_bits - ch1_bits_a;
    static constexpr unsigned ch1_mask_a = (1 << ch1_bits_a) - 1;
    static constexpr unsigned ch1_mask_b = (1 << ch1_bits_b) - 1;
    static constexpr unsigned ch2_mask = (1 << ch2_bits) - 1;

    static constexpr unsigned native_ch0_pos = total_bits - ch0_bits;
    static constexpr unsigned native_ch1_pos = native_ch0_pos - ch1_bits;

public:
    using value_type = type_from_bits_t<total_bits, false>;

protected:
    // These represent swapped positions (so normally upper_pos would be 1 for LE, and is 0 since we're swapped)
    static constexpr int upper_pos = estd::endian::native == estd::endian::little ? 0 : 1;
    static constexpr int lower_pos = estd::endian::native == estd::endian::little ? 1 : 0;

    // DEBT: type punning not always available this way
    union
    {
        // As indicated in above specialization rules, only 16-bit packed words come through here
        uint8_t value_[2];
        value_type native_;
    };

public:
    packed_word_base() = default;

    // DEBT: No upper masking
    // DEBT: byteswap here not optimal
    constexpr packed_word_base(unsigned ch0, unsigned ch1, unsigned ch2) :
        native_{estd::byteswap(static_cast<value_type>(ch0 << native_ch0_pos | ch1 << native_ch1_pos | ch2))}

        // NOTE: Not quite correct and also depends on architecture endianness
        //value_{ch0 << ch0_pos | ch1 >> ch1_pos, ((ch1 & ch1_mask_b) << ch1_pos) | ch2}
    {}

    template <unsigned Index>
    constexpr value_type channel() const
    {
        static_assert(Index <= 2, "Only 3 channels supported for this type");

        if constexpr(Index == 0)
        {
            return value_[upper_pos] >> ch0_pos;
        }
        else if constexpr(Index == 1)
        {
            const value_type upper = (value_[upper_pos] & ch1_mask_a) << 8;
            const uint8_t& lower = value_[lower_pos];

            return (upper | lower) >> ch1_pos;
        }
        else
        {
            return value_[lower_pos] & ch2_mask;
        }
    }
};

template <packed_word_options o, class IntSeq>
constexpr bool operator ==(
    const packed_word_base<o, IntSeq>& lhs,
    const packed_word_base<o, IntSeq>& rhs)
{
    return lhs.value() == rhs.value();
}

// Warning: it's up to you to determine if < or > is appropriate for your packed word.  We support it here
// in the spirit of low-level-ness

template <packed_word_options o, class IntSeq>
constexpr bool operator <(
    const packed_word_base<o, IntSeq>& lhs,
    const packed_word_base<o, IntSeq>& rhs)
{
    return lhs.value() < rhs.value();
}

template <packed_word_options o, class IntSeq>
constexpr bool operator >(
    const packed_word_base<o, IntSeq>& lhs,
    const packed_word_base<o, IntSeq>& rhs)
{
    return lhs.value() > rhs.value();
}

}}

}}

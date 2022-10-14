#pragma once

#include "../bits-temp.hpp"

namespace embr { namespace bits {

namespace internal {

template <class TInt, class TForwardIt>
inline TInt get_be_lsb_to_msb(const unsigned width, descriptor d, TForwardIt raw);

struct be_getter_base : getter_tag
{
    constexpr static int adjuster() { return 0; }

    constexpr static int adjuster(descriptor) { return 0; }
};

}

namespace detail {


template <unsigned bitpos, unsigned length>
struct getter<bitpos, length, big_endian, lsb_to_msb, lsb_to_msb,
    enable<internal::is_valid(bitpos, length) &&
           !internal::is_byte_boundary(bitpos, length) &&
           !internal::is_subbyte(bitpos, length)> > :
    internal::be_getter_base
{
    template <typename TReverseIt, typename TInt,
        estd::enable_if_t<(sizeof(TInt) > 1), bool> = true>
    static inline void get_assist(unsigned sz, TReverseIt& raw, TInt& v)
    {
        constexpr unsigned byte_width = byte_size();

        while(sz--)
        {
            v <<= byte_width;
            v |= (byte) *++raw;
        }
    }

    // Prep for uint8_t operation
    template <typename TForwardIt, typename TInt,
        estd::enable_if_t<(sizeof(TInt) <= 1), bool> = true>
    static inline void get_assist(unsigned, TForwardIt, TInt&)
    {
    }

    template <typename TForwardIt, typename TInt>
    static inline void get_legacy(descriptor d, TForwardIt raw, TInt& v)
    {
        const unsigned width = internal::width_deducer_lsb_to_msb(d);

        v = internal::get_be_lsb_to_msb<TInt>(width, d, raw);
    }

    template <typename TForwardIt, typename TInt>
    static inline void get_bytesize(descriptor d, TForwardIt raw, TInt& v)
    {
        typedef typename estd::iterator_traits<TForwardIt>::value_type byte_type;
        constexpr size_t byte_width = sizeof(byte_type) * byte_size();

        v >>= d.bitpos;

        // 'i' represents remaining bits after initial bitpos byte is processed
        int i = d.length - (byte_width - d.bitpos);

        auto remaining_bits = (byte_type)i;
        byte_type mask = (1 << remaining_bits) - 1;

        ++raw;

        v <<= remaining_bits;
        v |= (*raw & mask);
    }

    template <typename TForwardIt, typename TInt>
    static inline void get(descriptor d, TForwardIt raw, TInt& v)
    {
        typedef typename estd::iterator_traits<TForwardIt>::value_type byte_type;
        constexpr size_t byte_width = sizeof(byte_type) * byte_size();

        v = *raw;

        if(d.length <= 8)
        {
            get_bytesize(d, raw, v);
            return;
        }

        v >>= d.bitpos;

        // 'i' represents remaining bits after initial bitpos byte is processed
        int i = d.length - (byte_width - d.bitpos);

        // process middle bytes, excluding end
        get_assist(i / byte_width, raw, v);

        byte_type lsb_outside_bits;
        byte_type remainder_bits = i % byte_width;
        byte_type mask = (1 << remainder_bits) - 1;

        ++raw;

        v <<= remainder_bits;
        v |= (*raw & mask);
    }
};


// Byte boundary flavor
template <unsigned bitpos, unsigned length>
struct getter<bitpos, length, big_endian, lsb_to_msb, lsb_to_msb,
    enable<internal::is_valid(bitpos, length) &&
           internal::is_byte_boundary(bitpos, length) &&
           !internal::is_subbyte(bitpos, length)> > :
    internal::be_getter_base
{
    template <typename TReverseIt, typename TInt,
        estd::enable_if_t<(sizeof(TInt) > 1), bool> = true>
    static inline void get_assist(unsigned sz, TReverseIt& raw, TInt& v)
    {
        constexpr unsigned byte_width = byte_size();

        while(--sz)
        {
            v <<= byte_width;
            v |= (byte) *++raw;
        }
    }

    // Prep for uint8_t operation
    template <typename TForwardIt, typename TInt,
        estd::enable_if_t<(sizeof(TInt) <= 1), bool> = true>
    static inline void get_assist(unsigned, TForwardIt, TInt&)
    {
    }

    template <typename TForwardIt, typename TInt>
    static inline void get(descriptor d, TForwardIt raw, TInt& v)
    {
        typedef typename estd::iterator_traits<TForwardIt>::value_type byte_type;
        constexpr size_t byte_width = sizeof(byte_type) * byte_size();

        v = *raw;

        // process middle and end bytes
        get_assist(d.length / byte_width, raw, v);
    }
};


}

}}
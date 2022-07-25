#pragma once

#include "byte.hpp"

namespace embr { namespace bits {

namespace experimental {

constexpr unsigned max_bits()
{
    return byte_size();
}

constexpr bool is_valid(unsigned bitpos, unsigned length)
{
    return bitpos <= max_bits() && length > 0;
}

constexpr bool is_byte_boundary(unsigned bitpos, unsigned length)
{
    return bitpos == 0 && (length % byte_size() == 0);
}

constexpr bool is_subbyte(unsigned bitpos, unsigned length)
{
    return //(bitpos <= max_bits()) &&  // implicit in following line
        (length + bitpos <= 8);
}


constexpr bool is_valid(descriptor d)
{
    return is_valid(d.bitpos, d.length);
}

constexpr bool is_byte_boundary(descriptor d)
{
    return is_byte_boundary(d.bitpos, d.length);
}

constexpr bool is_subbyte(descriptor d)
{
    return is_subbyte(d.bitpos, d.length);
}

struct setter_tag {};

// NOTE: Remember, when using this 'enable' trick, specialization has to
// focus inside the 'enable' portion.  For example, "setter<0...." makes
// compiler mad

template <unsigned bitpos, unsigned length>
struct setter<bitpos, length, big_endian, lsb_to_msb, lsb_to_msb,
    enable<is_valid(bitpos, length) &&
        !is_byte_boundary(bitpos, length) &&
        !is_subbyte(bitpos, length)> > :
    setter_tag
{
    constexpr static int adjuster()
    {
        return 0;
    }

    // disabling to avoid false positives on compile - keeping overall specialization
    // to further put 'enable' through its paces
    /*
    template <typename TIt, typename TInt>
    static inline void set(TIt raw, TInt v)
    {
    } */
};

// byte boundary flavor means no bitpos/length processing required, so length_direction
// and resume_direction are ignored
template <unsigned bitpos, unsigned length, length_direction ld, resume_direction rd>
struct setter<bitpos, length, big_endian, ld, rd,
    enable<is_byte_boundary(bitpos, length) &&
        !is_subbyte(bitpos, length)> > :
    setter_tag
{
    constexpr static int adjuster()
    {
        return (length / byte_size()) - 1;
    }

    constexpr static int adjuster(descriptor d)
    {
        return (d.length / byte_size()) - 1;
    }

    template <typename TReverseIt, typename TInt>
    static inline void set(descriptor d, TReverseIt raw, TInt v)
    {
        constexpr unsigned byte_width = byte_size();
        unsigned sz = d.length / byte_width;

        while(sz--)
        {
            *raw-- = (byte) v;
            v >>= byte_width;
        }
    }


    template <typename TReverseIt, typename TInt>
    static inline void set(TReverseIt raw, TInt v)
    {
        constexpr unsigned byte_width = byte_size();
        constexpr unsigned _sz = length / byte_width;
        unsigned sz = _sz;

        while(sz--)
        {
            *raw-- = (byte) v;
            v >>= byte_width;
        }
    }
};

template <endianness e>
using byte_boundary_setter = setter<0, byte_size() * 2, e, lsb_to_msb>;


// for subbyte operations, endianness and resume_direction do not matter
template <unsigned bitpos, unsigned length, endianness e, length_direction ld, resume_direction rd>
struct setter<bitpos, length, e, ld, rd,
    enable<is_subbyte(bitpos, length)> > :
    setter_tag
{
    constexpr static int adjuster()
    {
        return 0;
    }

    constexpr static int adjuster(descriptor d)
    {
        return 0;
    }


    template <typename TIt, typename TInt>
    static inline void set(descriptor d, TIt raw, TInt v)
    {
        bits::set<no_endian, byte, ld>(d, raw, v);
    }

    template <typename TIt, typename TInt>
    static inline void set(TIt raw, TInt v)
    {
        // DEBT: Set up a bits::set with template values 
        bits::set<no_endian, byte, ld>(
            descriptor{bitpos, length}, raw, v);
    }
};

template <endianness e, length_direction ld>
using subbyte_setter = setter<0, byte_size(), e, ld>;


}

namespace internal {


template <typename TInt>
struct setter<TInt, big_endian, lsb_to_msb, lsb_to_msb>
{
    // EXPERIMENTAL
    template <unsigned bitpos, unsigned length, typename TIt>
    static inline void set(TIt raw, TInt v)
    {
        typedef experimental::setter<bitpos, length, big_endian, lsb_to_msb> _setter;

        _setter::set(raw + _setter::adjuster(), v);
    }

    template <typename TIt>
    static inline void set(descriptor d, TIt raw, TInt v)
    {
        // DEBT: Enable or disable these cases with compile time config, possibly enum-flag style

        if(experimental::is_subbyte(d))
        {
            typedef experimental::subbyte_setter<big_endian, lsb_to_msb> s;

            s::set(d, raw + s::adjuster(d), v);
        }
        else if(experimental::is_byte_boundary(d))
        {
            typedef experimental::byte_boundary_setter<big_endian> s;

            s::set(d, raw + s::adjuster(d), v);
        }
        else if(experimental::is_valid(d))
        {
            //typedef experimental::setter<1, 8, big_endian, lsb_to_msb> s;

            //general_setter::set(d, raw, v);
        }
        else
        {
            // DEBT: Perhaps some kind of error logging?
        }
    }
};


// FIX: This is an artifact of the old ambiguous "byte means no endian" mechanism
// it's kept because we haven't actually implemented a proper BE setter yet, so this
// holds the fort until then.  Clearly these are basically fake endian operations,
// thus the FIX and not just DEBT
template <>
struct setter<byte, big_endian, lsb_to_msb, msb_to_lsb>
{
    template <typename TIt>
    static inline void set(descriptor d, TIt raw, byte v)
    {
        bits::set<no_endian, byte, lsb_to_msb>(d, raw, v);
    }

    template <typename TIt>
    static inline void set(unsigned, descriptor d, TIt raw, byte v)
    {
        set(d, raw, v);
    }
};

// DEBT: Devise a way for unit test to test native and non native flavors

template <typename TInt>
struct setter<TInt, big_endian, no_direction>
{
    static inline void set(byte* raw, TInt v)
    {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        set_native(raw, v);
#else
        set_be(raw, v);
#endif
    }
};


}

}}
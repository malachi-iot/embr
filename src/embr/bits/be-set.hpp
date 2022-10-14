#pragma once

#include "byte.hpp"
#include "bits-temp.hpp"

#include "detail/be-set.hpp"

namespace embr { namespace bits {

template <>
struct setter<big_endian, lsb_to_msb, lsb_to_msb>
{
    template <unsigned bitpos, unsigned length, typename TIt, typename TInt>
    static inline void set(TIt raw, TInt v)
    {
        typedef detail::setter<bitpos, length, big_endian, lsb_to_msb> _setter;

        _setter::set(raw + _setter::adjuster(), v);
    }

    template <typename TIt, typename TInt>
    static inline void set(descriptor d, TIt raw, TInt v)
    {
        // DEBT: Enable or disable these cases with compile time config, possibly enum-flag style

        if(internal::is_subbyte(d))
        {
            typedef experimental::subbyte_setter<lsb_to_msb> s;

            s::set(d, raw + s::adjuster(d), v);
        }
        else if(internal::is_byte_boundary(d))
        {
            typedef experimental::byte_boundary_setter<big_endian> s;

            s::set(d, raw + s::adjuster(d), v);
        }
        else if(internal::is_valid(d))
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
struct setter<big_endian, lsb_to_msb, msb_to_lsb>
{
    template <typename TIt, typename TInt>
    static inline void set(descriptor d, TIt raw, TInt v)
    {
        bits::set<no_endian, TInt, lsb_to_msb>(d, raw, v);
    }

    template <typename TIt, typename TInt>
    static inline void set(unsigned, descriptor d, TIt raw, TInt v)
    {
        set(d, raw, v);
    }
};

// DEBT: Devise a way for unit test to test native and non native flavors

template <>
struct setter<big_endian, no_direction>
{
    template <typename TInt>
    static inline void set(byte* raw, TInt v)
    {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        set_native(raw, v);
#else
        set_be(raw, v);
#endif
    }
};


}}

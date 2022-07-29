#pragma once

#include "descriptor.h"

namespace embr { namespace bits {

template <endianness e, length_direction direction = default_direction, resume_direction rd = direction,
    class TBase = internal::provider<e, internal::base<uint8_t> > >
class encoder : public TBase
{
    typedef TBase base_type;
    typedef typename base_type::value_type byte_type;
    typedef unsigned index_type;

protected:
    // Needed for layer1
    // FIX: Need to SFINEA-style disable/enable this based on whether we actually are in layer1 mode
    // that might be easiest by using a pure forwarding constructor all the way down
    encoder() = default;

public:
    encoder(byte_type* raw) : base_type(raw) {}

    template <class TInt>
    inline void set(index_type index, descriptor d, TInt v)
    {
        typedef internal::setter<TInt, e, direction, rd> setter;

        setter::set(d, base_type::data() + index, v);
    }

    template <class TInt>
    inline void set(index_type index, TInt v)
    {
        typedef internal::setter<TInt, e, direction> setter;

        setter::set(base_type::data() + index, v);

        /*
         * This works just fine, but does more masking/shifting legwork than necessary
        constexpr unsigned byte_size = 8;
        constexpr unsigned int_size = sizeof(v) * byte_size;

        set(index, descriptor{direction == lsb_to_msb ? 0 : byte_size - 1, int_size}, v); */

        //embr::bits::set<e, TInt>(base_type::data() + index, v);
    }
};

namespace layer1 {

// We do it this way so that the default constructor is now visible
template <endianness e, size_t N, length_direction direction = default_direction, resume_direction rd = direction>
class encoder : public embr::bits::encoder<e, direction, rd,
    internal::provider<e, estd::array<uint8_t, N> > >
{
    typedef embr::bits::encoder<e, direction, rd, estd::array<uint8_t, N> > base_type;
};

}


}}
#pragma once

#include "descriptor.h"

namespace embr { namespace bits {

// DEBT: Un-hardcode the '8'
template <endianness e, length_direction direction = default_direction, resume_direction rd = direction,
    class TBase = internal::provider<e, internal::base<const byte> > >
class decoder : public TBase
{
    typedef TBase base_type;
    typedef typename base_type::value_type byte_type;

protected:
    // Needed for layer1
    decoder() = default;

public:
    decoder(byte_type* raw) : base_type(raw) {}

    template <class TInt>
    inline TInt get(int index, descriptor d) const
    {
        return bits::get<e, TInt, direction, rd>(d, base_type::data() + index);
    }

    template <class TInt>
    inline TInt get(int index)
    {
        constexpr unsigned byte_size = 8;

        return get<TInt>(index,
                         descriptor{direction == lsb_to_msb ? 0 : byte_size - 1, sizeof(TInt) * byte_size});
    }

    // EXPERIMENTAL
    template <unsigned bitpos, unsigned length>
    inline word<length> get(unsigned bytepos)
    {
        typedef word<length> word_type;

        typedef internal::getter<typename word_type::type, e, direction, rd> getter;

        return getter::template get_adjusted<bitpos, length>(base_type::data() + bytepos);
    }
};

namespace layer1 {

template <endianness e, size_t N, length_direction direction = default_direction, resume_direction rd = direction>
class decoder : public embr::bits::decoder<e, direction, rd,
    internal::provider<e, estd::array<uint8_t, N> > >
{
    typedef embr::bits::decoder<e, direction, rd, estd::array<uint8_t, N> > base_type;
};

}

}}
#pragma once

#include "descriptor.h"

namespace embr { namespace bits {

template <endianness e, length_direction direction = default_direction, resume_direction rd = direction,
    class TBase = internal::provider<e, internal::base<const byte> > >
class decoder : public TBase
{
    typedef TBase base_type;
    typedef typename base_type::value_type byte_type;

public:
    ESTD_CPP_FORWARDING_CTOR(decoder)

    template <class TInt>
    inline TInt get(int index, descriptor d) const
    {
        return bits::get<e, TInt, direction, rd>(d, base_type::data() + index);
    }

    template <class TInt>
    inline TInt get(int index)
    {
        return bits::get<e, TInt>(base_type::data() + index);
    }

    // EXPERIMENTAL
    template <unsigned bitpos, unsigned length>
    inline word<length> get(unsigned bytepos) const
    {
        typedef word<length> word_type;

        typedef getter<e, direction, rd> getter;

        return getter::template get<bitpos, length, typename word_type::type>(base_type::data() + bytepos);
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

#define EMBR_BITS_DECODER_GETTER(name, offset, bitpos, length) \
embr::word<length> name() const                               \
{                                                   \
    return base_type::template get<bitpos, length>(offset);   \
}

}}
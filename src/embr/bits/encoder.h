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
    typedef setter<e, direction, rd> setter_type;

public:
    ESTD_CPP_FORWARDING_CTOR(encoder)

    // UNTESTED
    template <unsigned bitpos, unsigned length, class TInt>
    inline void set(index_type index, TInt v)
    {
        setter_type::template set<bitpos, length>(base_type::data() + index, v);
    }

    template <class TInt>
    inline void set(index_type index, descriptor d, TInt v)
    {
        setter_type::set(d, base_type::data() + index, v);
    }

    template <class TInt>
    inline void set(index_type index, TInt v)
    {
        bits::set<e>(base_type::data() + index, v);

        /*
         * This works just fine, but does more masking/shifting legwork than necessary
        constexpr unsigned byte_size = 8;
        constexpr unsigned int_size = sizeof(v) * byte_size;

        set(index, descriptor{direction == lsb_to_msb ? 0 : byte_size - 1, int_size}, v); */

        //embr::bits::set<e, TInt>(base_type::data() + index, v);
    }
};

#if __cpp_alias_templates
namespace layer1 {

template <endianness e, size_t N, length_direction direction = default_direction, resume_direction rd = direction>
using encoder = embr::bits::encoder<e, direction, rd,
    internal::provider<e, estd::array<uint8_t, N> > >;

}

namespace layer2 {

template <endianness e, size_t N, length_direction direction = default_direction>
using encoder = embr::bits::encoder<e, direction, direction,
    internal::provider<e, estd::layer2::array<uint8_t, N> > >;


}
#endif

#define EMBR_BITS_ENCODER_SETTER(name, offset, bitpos, length) \
void name(unsigned v)                         \
{                                                   \
    base_type::template set<bitpos, length>(offset, v);  \
}

}}
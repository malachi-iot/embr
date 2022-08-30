#pragma once

#include <estd/cstdint.h>

#include <estd/array.h>

#include "decoder.h"
#include "encoder.h"

#include "../platform/guard-in.h"

namespace embr { namespace bits {


/// Utility class combining encoder and decoder together.  Name still in flux
/// @tparam e
/// @tparam direction
template <endianness e, length_direction direction = default_direction, resume_direction rd = direction,
    class TBase = internal::provider<e, estd::layer2::array<uint8_t, 8> > >
class material : public decoder<e, direction, rd, encoder<e, direction, rd, TBase> >
{
    typedef decoder<e, direction, rd, encoder<e, direction, rd, TBase> > base_type;
    typedef typename base_type::value_type byte_type;

protected:
    // Needed for layer1
    material() = default;

public:
    material(byte_type* raw) : base_type(raw) {}
};



// Almost exactly like regular encoder/decoder but these *imply* an upper boundary, though
// it is not enforced.  Also possibly useful is since we derive from array, you get access to size()
namespace layer2 {

// We do it this way so that the default constructor is now visible
template <endianness e, size_t N, length_direction direction = default_direction>
class encoder : public embr::bits::encoder<e, direction, direction,
    internal::provider<e, estd::layer2::array<uint8_t, N> > >
{
    //typedef embr::bits::encoder<e, direction, estd::layer2::array<uint8_t, N> > base_type;
};

template <endianness e, size_t N, length_direction direction = default_direction, resume_direction rd = direction>
class decoder : public embr::bits::decoder<e, direction, rd,
    internal::provider<e, estd::layer2::array<uint8_t, N> > >
{
    //typedef embr::bits::decoder<e, direction, estd::layer2::array<uint8_t, N> > base_type;
};

}

namespace experimental {

// NOTE: Unsure if I like having direction here, but seems useful
template <unsigned bitpos_, unsigned length_, length_direction direction_ = default_direction>
struct bit_traits
{
    static CONSTEXPR unsigned bitpos = bitpos_;
    static CONSTEXPR unsigned length = length_;
    static CONSTEXPR length_direction direction = direction_;

    typedef word<length> word_type;

    // A little clumsy, remember we need an indicator of the full containing word bit size
    // where above word_type is the subset word.  So we add length_ and bitpos_ together,
    // which will nudge bit count high enough for descriptor_base to specialize and choose
    // the bitness_type big enough to hold on to the desired values
    typedef internal::descriptor_base<length_ + bitpos_> descriptor_type;
    typedef typename descriptor_type::uint_type descriptor_uint_type;

    static ESTD_CPP_CONSTEXPR_RET descriptor_type get_descriptor()
    {
        return descriptor_type{bitpos, length};
    }

    static ESTD_CPP_CONSTEXPR_RET descriptor_uint_type mask() { return (1 << length) - 1; }
    static ESTD_CPP_CONSTEXPR_RET descriptor_uint_type shifted_mask() { return mask() << bitpos; }

#ifdef FEATURE_CPP_DEFAULT_TARGS
    template <class T = descriptor_uint_type, length_direction d = direction_>
#else
    template <class T, length_direction d>
#endif
    static ESTD_CPP_CONSTEXPR_RET word_type get(const T* raw)
    {
        // FIX: msb_to_lsb mode still needed here
        return d == lsb_to_msb ? ((*raw >> bitpos) & mask()) : 0;
    }

#ifdef FEATURE_CPP_DEFAULT_TARGS
    template <class T = descriptor_uint_type, length_direction d = direction_>
#else
    template <class T, length_direction d>
#endif
    static ESTD_CPP_CONSTEXPR_RET descriptor_uint_type set(T* raw, word_type value)
    {
        return d == lsb_to_msb ?
            (*raw &= ~shifted_mask()) |= estd::to_integer<descriptor_uint_type>(value) << bitpos :
            0;
    }

};

}

}}


#include "../platform/guard-out.h"

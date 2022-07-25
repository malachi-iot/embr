#pragma once

#include <estd/cstdint.h>

#include <estd/array.h>

#include "../word.h"

#include "../fwd/bits/bits.h"

#include "../platform/guard-in.h"

namespace embr { namespace bits {


namespace internal {

// DEBT: Devise a way for unit test to test native and non native flavors

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#endif


template <class TByte, class TInt>
inline void set_native(TByte* raw, TInt v) { *((TInt*)raw) = v; }

template <class TInt, class TByte>
inline TInt get_native(TByte* raw, TInt v) { return *((TInt*)raw); }


/// Unshifted mask reflecting length
/// @return
template <typename T, size_t bitness>
constexpr T mask(const descriptor_base<bitness>& d)
{
    return (1 << d.length) - 1;
}

template <typename T, size_t bitness>
constexpr T shifted_mask(const descriptor_base<bitness>& d)
{
    return mask<T>(d) << d.bitpos;
}


template <size_t bitness>
struct descriptor_base<bitness, estd::internal::Range<(bitness <= 8)> >
{
    typedef uint8_t bitness_type;
    typedef uint8_t uint_type;

    const uint8_t bitpos : 3;      ///< 0-based, inclusive - starting from lsb
    const uint8_t length : 3;      ///< in bits.  @see length_direction

    descriptor_base() = default;

    constexpr descriptor_base(bitness_type bitpos, bitness_type length) :
        bitpos{bitpos}, length{length} {}

    // NOTE: Be careful, narrowing happens here and undefined behavior occurs if it overflows
    template <size_t bitness2>
    constexpr descriptor_base(const internal::descriptor_base<bitness2>& copy_from) :
        bitpos{(bitness_type)copy_from.bitpos}, length{(bitness_type)copy_from.length}
        {}
};

template <size_t bitness>
struct descriptor_base<bitness, estd::internal::Range<
    (bitness > 8 && bitness <= estd::numeric_limits<unsigned short>::max())> >
{
    typedef unsigned short bitness_type;
    // DEBT: Deduce this more closely based on bitness itself
    typedef unsigned uint_type;

    const unsigned short bitpos;      ///< 0-based, inclusive - starting from lsb
    const unsigned short length;      ///< in bits.  @see length_direction

    descriptor_base() = default;

    constexpr descriptor_base(bitness_type bitpos, bitness_type length) :
        bitpos{bitpos}, length{length} {}


    template <size_t bitness2>
    constexpr descriptor_base(const internal::descriptor_base<bitness2>& copy_from) :
        bitpos{copy_from.bitpos}, length{copy_from.length}
    {}
};


template <size_t bitness1, size_t bitness2>
constexpr bool operator==(descriptor_base<bitness1> lhs, descriptor_base<bitness2> rhs)
{
    return lhs.length == rhs.length && lhs.bitpos == rhs.bitpos;
}

}

struct descriptor : internal::descriptor_base<64>
{
    typedef internal::descriptor_base<64> base_type;

    /// Integer type used to track bit position and bit length
    typedef typename base_type::bitness_type bitness_type;
    /// Integer type of minimum size necessary for descriptor to be valid
    typedef typename base_type::uint_type uint_type;

    // DEBT: Needs a cleanup
    constexpr descriptor(unsigned bitpos, unsigned length) : base_type{(bitness_type)bitpos, (bitness_type)length} {}
    constexpr descriptor(int bitpos, int length) : base_type{(bitness_type)bitpos, (bitness_type)length} {}

    template <size_t bitness>
    constexpr descriptor(const internal::descriptor_base<bitness>& copy_from) :
        base_type(copy_from) {}

    //const unsigned bitpos;      ///< 0-based, inclusive - starting from lsb
    //const unsigned length;      ///< in bits.  @see length_direction
};

namespace internal {

// Using this instead of layer2 array because there are time we don't even want to imply that
// we are observing an upper bound
template <class T>
class base
{
public:
    typedef T value_type;

protected:
    T* const raw;

    base(value_type* raw) : raw{raw} {}

    T* data() { return raw; }
    const T* data() const { return raw; }
};

// A kind of universal base class/tag denoting an underlying collection
// generally structured as endianness 'e'
template <endianness _e, class TBase>
class provider : public TBase
{
    typedef TBase base_type;

protected:
    ESTD_CPP_FORWARDING_CTOR(provider)

public:
    static constexpr endianness e = _e;
};

}

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
        typedef internal::getter<TInt, e, direction, rd> getter;

        return getter::get_adjusted(d, base_type::data() + index);
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

namespace layer1 {

// We do it this way so that the default constructor is now visible
template <endianness e, size_t N, length_direction direction = default_direction, resume_direction rd = direction>
class encoder : public embr::bits::encoder<e, direction, rd,
    internal::provider<e, estd::array<uint8_t, N> > >
{
    typedef embr::bits::encoder<e, direction, rd, estd::array<uint8_t, N> > base_type;
};

template <endianness e, size_t N, length_direction direction = default_direction, resume_direction rd = direction>
class decoder : public embr::bits::decoder<e, direction, rd,
    internal::provider<e, estd::array<uint8_t, N> > >
{
    typedef embr::bits::decoder<e, direction, rd, estd::array<uint8_t, N> > base_type;
};

}

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
    static constexpr unsigned bitpos = bitpos_;
    static constexpr unsigned length = length_;
    static constexpr length_direction direction = direction_;

    typedef word<length> word_type;

    // A little clumsy, remember we need an indicator of the full containing word bit size
    // where above word_type is the subset word.  So we add length_ and bitpos_ together,
    // which will nudge bit count high enough for descriptor_base to specialize and choose
    // the bitness_type big enough to hold on to the desired values
    typedef internal::descriptor_base<length_ + bitpos_> descriptor_type;
    typedef typename descriptor_type::uint_type descriptor_uint_type;

    static constexpr descriptor_type get_descriptor()
    {
        return descriptor_type{bitpos, length};
    }

    static constexpr descriptor_uint_type mask() { return (1 << length) - 1; }
    static constexpr descriptor_uint_type shifted_mask() { return mask() << bitpos; }

    template <class T = descriptor_uint_type, length_direction d = direction_>
    static constexpr word_type get(const T* raw)
    {
        // FIX: msb_to_lsb mode still needed here
        return d == lsb_to_msb ? ((*raw >> bitpos) & mask()) : 0;
    }

    template <class T = descriptor_uint_type, length_direction d = direction_>
    static constexpr descriptor_uint_type set(T* raw, word_type value)
    {
        return d == lsb_to_msb ?
            (*raw &= ~shifted_mask()) |= estd::to_integer<descriptor_uint_type>(value) << bitpos :
            0;
    }

};

}

}}


#include "../platform/guard-out.h"

#pragma once

#include <estd/cstdint.h>

#include "../../word.h"

#include "../../fwd/bits/bits.h"

#include "../../platform/guard-in.h"

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
ESTD_CPP_CONSTEXPR_RET T mask(const descriptor_base<bitness>& d)
{
    return (1 << d.length) - 1;
}

template <typename T, size_t bitness>
ESTD_CPP_CONSTEXPR_RET T shifted_mask(const descriptor_base<bitness>& d)
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

#if __cplusplus >= 201103L
    descriptor_base() = default;

    constexpr descriptor_base(bitness_type bitpos, bitness_type length) :
        bitpos{bitpos}, length{length} {}
#else
    descriptor_base() { }

    inline descriptor_base(bitness_type bitpos, bitness_type length) :
        bitpos(bitpos), length(length) {}
#endif

    // NOTE: Be careful, narrowing happens here and undefined behavior occurs if it overflows
    template <size_t bitness2>
    ESTD_CPP_CONSTEXPR_RET descriptor_base(const internal::descriptor_base<bitness2>& copy_from) :
        bitpos((bitness_type)copy_from.bitpos), length((bitness_type)copy_from.length)
    {}
};

template <size_t bitness>
struct descriptor_base<bitness, estd::internal::Range<
    (bitness > 8 && bitness <=
#ifdef __cpp_constexpr
        estd::numeric_limits<unsigned short>::max()
#else
        USHRT_MAX
#endif
    )> >
{
    typedef unsigned short bitness_type;
    // DEBT: Deduce this more closely based on bitness itself
    typedef unsigned uint_type;

    const unsigned short bitpos;      ///< 0-based, inclusive - starting from lsb
    const unsigned short length;      ///< in bits.  @see length_direction

#if __cplusplus >= 201103L
    descriptor_base() = default;

    constexpr descriptor_base(bitness_type bitpos, bitness_type length) :
        bitpos{bitpos}, length{length} {}
#else
    descriptor_base() { };

    inline descriptor_base(bitness_type bitpos, bitness_type length) :
        bitpos(bitpos), length(length) {}
#endif


    template <size_t bitness2>
    ESTD_CPP_CONSTEXPR_RET descriptor_base(const internal::descriptor_base<bitness2>& copy_from) :
        bitpos(copy_from.bitpos), length(copy_from.length)
    {}
};


template <size_t bitness1, size_t bitness2>
ESTD_CPP_CONSTEXPR_RET bool operator==(descriptor_base<bitness1> lhs, descriptor_base<bitness2> rhs)
{
    return lhs.length == rhs.length && lhs.bitpos == rhs.bitpos;
}


// Using this instead of layer2 array because there are time we don't even want to imply that
// we are observing an upper bound
// DEBT: We can replace this with internal::span_base
template <class T>
class base
{
public:
    typedef T value_type;

protected:
    T* const raw;

    base(value_type* raw) : raw(raw) {}

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
    static CONSTEXPR endianness e = _e;
};


}

}}

#include "../../platform/guard-out.h"

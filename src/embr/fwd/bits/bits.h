// NOTE: Not all of the 'bits' lib is properly in embr folder yet
#pragma once

#include <estd/cstdint.h>
#include <estd/internal/value_evaporator.h>

namespace embr { namespace bits {

// DEBT: Use estd::byte instead
typedef unsigned char byte;
/// @return in bits
constexpr unsigned byte_size()
{
#ifdef UCHAR_WIDTH
    return UCHAR_WIDTH;
#else
    return estd::numeric_limits<unsigned char>::digits;
#endif
}

enum endianness
{
    no_endian,                  ///< Operations which only happen within a byte, therefore no endianness is involved
    big_endian,
    little_endian,

    unspecified_endian,         ///< For internal use only
};

/// Denotes which direction starting from bitpos to evaluate length
enum length_direction
{
    lsb_to_msb,         ///< For bitpos length, starts at LSB 0 and ends at MSB (inclusive)
                        ///< This inclusive MSB position is from where bit material begins
    msb_to_lsb,         ///< For bitpos length, starts at specified MSB (inclusive) and ends at LSB 0.
                        ///< This inclusive MSB position is from where bit material begins
                        ///< For resume, starts at uppermost LSB (inclusive).  For 8-bit byte, that would be '7'

    no_direction,       ///< For scenarios which are on byte boundaries. bitpos is not used

    default_direction = lsb_to_msb
};

/// Denotes direction from which to resume the last bits of a
/// multi-byte int.
/// i.e. given a 12 bit length in a 16 bit integer, msb_to_lsb means
/// start from bit 7 going down to bit 4 for storing the remaining 4 bits
typedef length_direction resume_direction;

namespace internal {

///
/// @tparam bitness bitness of the word that this describes
/// @remarks Byte streams are not quite interchangeable with 'word' in this context.
///          In this case, byte streams really are a bundle of bytes so 'word' bitness
///          would be technically be 8 (or whatever byte_size() is for this platform)
template <size_t bitness, typename = estd::internal::Range<true> >
struct descriptor_base;

/// Utility for greater than, less than, etc. comparisons
/// @tparam e
/// @tparam greater_than is this comparison helper in greater than or less than mode
/// @tparam equal_to is this comparison helper in >= & <= mode, or > and < mode
/// @remarks struct used instead of specific comparison method(s) so that we can specialize
/// primarily on endianness
/// expect an 'eval' which takes 'provider', and ...
/// TODO: Perhaps also the more explicit templated iterator flavor
/// NOTE: Only works on byte boundaries at this time
template <endianness e, bool greater_than, bool equal_to>
struct compare;

// NOTE: Looks like it would be helpful to move getter/setter out of internal, though
// I think we'd prefer to ditch 'TInt' before doing so

template <class TInt, endianness e, length_direction ld, resume_direction rd = ld>
struct getter;

template <class TInt, endianness e, length_direction ld, resume_direction rd = ld>
struct setter;

}

/// contains length and sub-byte bit position of data
struct descriptor;

template <endianness e, class TInt, length_direction d = default_direction,
    resume_direction rd = d, class TByte>
TInt get(descriptor, const TByte* raw);

template <endianness e, class TInt, length_direction d = default_direction,
    resume_direction rd = d, class TByte>
void set(descriptor, TByte* raw, TInt v);

// Follows are regular non-fancy endian conversions.  Just included for a consistent API

template <endianness e, class TInt, class TByte>
TInt get(const TByte* raw);

template <endianness e, class TInt, class TByte>
void set(TByte* raw, TInt v);


namespace experimental {

// copy/paste of our 'Range' technique
template <bool>
struct enable;

// "higher definition" setter and getter, so that we can specialize even more

template <unsigned bitpos, unsigned length, endianness e, length_direction ld, resume_direction rd = ld, typename = enable<true> >
struct setter;

template <unsigned bitpos, unsigned length, endianness e, length_direction ld, resume_direction rd = ld, typename = enable<true> >
struct getter;


// The following two discrete methods are good to have, but actually somewhat clumsy compared to
// current 'v2' getter::get and setter::set since they can retain endianness and length direction
// while varying bitpos, length etc

template <unsigned bitpos, unsigned length, endianness e,
    length_direction d = default_direction, resume_direction rd = d,
    typename TInt, typename TIt>
void get(TIt raw, TInt& v);

template <unsigned bitpos, unsigned length, endianness e,
    length_direction d = default_direction, resume_direction rd = d,
    typename TInt, typename TIt>
void set(TIt raw, TInt v);


}


}}
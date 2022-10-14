// NOTE: Not all of the 'bits' lib is properly in embr folder yet
#pragma once

#include <estd/cstdint.h>
#include <estd/internal/value_evaporator.h>

#include "base.h"
#include "get.h"
#include "set.h"

namespace embr { namespace bits {

// DEBT: Use estd::byte instead
typedef unsigned char byte;

#ifdef __cpp_constexpr
/// @return in bits
constexpr unsigned byte_size()
{
#ifdef UCHAR_WIDTH
    return UCHAR_WIDTH;
#else
    return estd::numeric_limits<unsigned char>::digits;
#endif
}
#else
// DEBT: We assume UCHAR_WIDTH presence for c++03
// DEBT: Don't love faking out a constexpr function with this lower case
#define byte_size() UCHAR_WIDTH
#endif



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

}

// Follows are regular non-fancy endian conversions.  Just included for a consistent API

template <endianness e, class TInt, class TByte>
TInt get(const TByte* raw);

template <endianness e, class TInt, class TByte>
void set(TByte* raw, TInt v);


namespace detail {

// copy/paste of our 'Range' technique
template <bool>
struct enable;

// "higher definition" setter and getter, so that we can specialize even more

template <unsigned bitpos, unsigned length, endianness e, length_direction ld, resume_direction rd = ld, typename = enable<true> >
struct setter;

template <unsigned bitpos, unsigned length, endianness e, length_direction ld, resume_direction rd = ld, typename = enable<true> >
struct getter;

}

namespace experimental {
// The following two discrete methods are good to have, but actually somewhat clumsy compared to
// current 'v2' getter::get and setter::set since they can retain endianness and length direction
// while varying bitpos, length etc

template <unsigned bitpos, unsigned length, endianness e,
#ifdef FEATURE_CPP_DEFAULT_TARGS
    length_direction d = default_direction, resume_direction rd = d,
#else
    length_direction d, resume_direction rd,
#endif
    typename TInt, typename TIt>
void get(TIt raw, TInt& v);

template <unsigned bitpos, unsigned length, endianness e,
#ifdef FEATURE_CPP_DEFAULT_TARGS
    length_direction d = default_direction, resume_direction rd = d,
#else
    length_direction d, resume_direction rd,
#endif
    typename TInt, typename TIt>
void set(TIt raw, TInt v);

}

namespace internal {

// These assisters are used to do the byte-oriented endian setting and getting
// Bit boundary operations are NOT handled in the assist mechanism.  These might
// be 1:1 with "normal" unfancy endian getters/setters, not investigated at this time

// DEBT: Clean up naming, can't name set_assist as that currently is a set
// of functions we're displacing
// DEBT: Make unit tests also
template <endianness e, typename TInt>
struct set_assist;

// NOTE: Not 100% sure we need this level of fanciness, I think burying the assisters
// in the getters and setters may be enough
template <endianness e, bool byte_boundary, typename TInt, typename Enabled = void>
struct get_assister;



}


}}
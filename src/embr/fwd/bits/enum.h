#pragma once

namespace embr { namespace bits {

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


}}
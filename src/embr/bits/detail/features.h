#pragma once

// Does stricter runtime sanity checks during bit set operations, such as ensuring
// nobody's actually calling the 8-bit shift helper on an integer who itself is only
// 8 bits.  These checks call abort() but we expect them to NEVER happen.  Turn off
// for that extra tidbit of performance
#ifndef FEATURE_ESTD_STRICT_BITS_SET
#define FEATURE_ESTD_STRICT_BITS_SET 1
#endif
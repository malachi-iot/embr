#pragma once

// Two issues:
// 1. No gauruntee that a union between 8 and 16-bit header structures will pack the bits the same way
// 2. null = max of value, but injecting this bit in there interrupts that
// One thing though is the handles themselves are not bit packed meaning regular ordering and packing rules may
// apply so we could manually bit-field it up.  Removed all usages of this guy being the non starter he is and
// doing manual bitmasking instead
#ifndef FEATURE_EMBR_GC_EXP
#define FEATURE_EMBR_GC_EXP 0
#endif

// Brute force all handles to be 16 bit instead of 8 bit.  Will break a lot of tests but technically should work
// Code which does this will be sloppy for a while
#ifndef FEATURE_EMBR_GC_16BIT_EXP
#define FEATURE_EMBR_GC_16BIT_EXP 0
#endif
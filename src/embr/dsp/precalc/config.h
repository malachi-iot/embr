#pragma once

#if ESP_PLATFORM
#include <sdkconfig.h>

// FIX: ESP-IDF seems to treat 'y' here as a 1, but regular GCC doesn't.  This is too wide
// a descrepency, I am not understanding something
#define FEATURE_EMBR_DSP_PRECALC_TABLE CONFIG_EMBR_DSP_PRECALC_TABLE
#endif

#if ESP_PLATFORM && FEATURE_EMBR_DSP_PRECALC_TABLE
#define FEATURE_EMBR_DSP_PRECALC_TABLE_STATIC CONFIG_EMBR_DSP_PRECALC_TABLE_STATIC
#define EMBR_DSP_PRECALC_TABLE_SZ   CONFIG_EMBR_DSP_PRECALC_TABLE_SZ

#else

// 14MAR25 FIX: Broken/unfinished, only static mode works right now
#ifndef FEATURE_EMBR_DSP_PRECALC_TABLE_STATIC
#define FEATURE_EMBR_DSP_PRECALC_TABLE_STATIC 1
#endif

#ifndef FEATURE_EMBR_DSP_PRECALC_TABLE
#define FEATURE_EMBR_DSP_PRECALC_TABLE 1
#endif

#ifndef EMBR_DSP_PRECALC_TABLE_SZ
#define EMBR_DSP_PRECALC_TABLE_SZ   1024
#endif

#endif  // ESP_PLATFORM && FEATURE_EMBR_DSP_PRECALC_TABLE

// 10NOV26 MB DEBT - EMBR_DSP_AGGRESSIVE_INLINE and friends belong elsewhere
#ifndef EMBR_DSP_AGGRESSIVE_INLINE
#define EMBR_DSP_AGGRESSIVE_INLINE 1
#endif

#if EMBR_DSP_AGGRESSIVE_INLINE && !defined(__clang__)
#define EMBR_DSP_INLINE inline __attribute__((always_inline, optimize("tree-sra")))
#define EMBR_DSP_INLINE_FLATTEN inline __attribute__((always_inline, flatten, optimize("tree-sra")))
#else
#define EMBR_DSP_INLINE
#define EMBR_DSP_INLINE_FLATTEN
#endif

#ifndef EMBR_DSP_FAST_ROUND
#define EMBR_DSP_FAST_ROUND 1
#endif

// Enforcer of sin_lookup >= 0, costs some cycles though
#ifndef EMBR_DSP_FAST_ROUND_STRICT
#define EMBR_DSP_FAST_ROUND_STRICT 0
#endif

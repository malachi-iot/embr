#pragma once

#if ESP_PLATFORM
#include <sdkconfig.h>

// FIX: ESP-IDF seems to treat 'y' here as a 1, but regular GCC doesn't.  This is too wide
// a descrepency, I am not understanding something
#define FEATURE_EMBR_DSP_PRECALC_TABLE CONFIG_EMBR_DSP_PRECALC_TABLE
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
#define EMBR_DSP_PRECALC_TABLE_SZ   4096
#endif

#endif

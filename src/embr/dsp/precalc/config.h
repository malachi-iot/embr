#pragma once

#if ESP_PLATFORM
#include <sdkconfig.h>
#endif

#if ESP_PLATFORM
// FIX: ESP-IDF seems to treat 'y' here as a 1, but regular GCC doesn't.  This is too wide
// a descrepency, I am not understanding something
#define EMBR_DSP_PRECALC_TABLE CONFIG_EMBR_DSP_PRECALC_TABLE
#else
#define EMBR_DSP_PRECALC_TABLE 1
#endif

#if CONFIG_EMBR_DSP_PRECALC_TABLE_SZ
#define EMBR_DSP_PRECALC_TABLE_SZ   CONFIG_EMBR_DSP_PRECALC_TABLE_SZ
#elif !EMBR_DSP_PRECALC_TABLE_SZ
#define EMBR_DSP_PRECALC_TABLE_SZ   4096
#endif


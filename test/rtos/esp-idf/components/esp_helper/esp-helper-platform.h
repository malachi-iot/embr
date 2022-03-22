#pragma once

#include <estd/internal/platform.h>

#include "esp_event.h"

// even though 'esp_event_loop_create_default' exists in previous
// versions, something appears to be off about it
// https://www.gitmemory.com/issue/espressif/esp-idf/3838/515464313
// https://github.com/espressif/esp-idf/issues/3838
// https://github.com/espressif/esp-idf/issues/668
// so until that's all worked out, we feature flag it
// Also, it appears the events it connects to aren't fully defined
// until 4.0.0 anyway, which is awkward
#if ESTD_IDF_VER >= ESTD_IDF_VER_4_0_0
// represents 3.3.0+ "recommended" event loop
// esp_event_loop_create_default / esp_event_loop_create
// vs
// esp_event_loop_init
// naming this as such because both are named event, with previous
// one being legacy
#define FEATURE_IDF_DEFAULT_EVENT_LOOP
#else
#include "esp_event_loop.h"
#endif

#define ENABLE_IDF_LEGACY_EVENT ESTD_IDF_VER < ESTD_IDF_VER_3_3_0

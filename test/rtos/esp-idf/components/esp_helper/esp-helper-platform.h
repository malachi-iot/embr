#pragma once

#include <estd/internal/platform.h>

#include "esp_event.h"
#include "esp_event_loop.h"

// even though 'esp_event_loop_create_default' exists in previous
// versions, something appears to be off about it
// https://www.gitmemory.com/issue/espressif/esp-idf/3838/515464313
// https://github.com/espressif/esp-idf/issues/3838
// https://github.com/espressif/esp-idf/issues/668
// so until that's all worked out, we feature flag it
#if ESTD_IDF_VER >= ESTD_IDF_VER_4_0_0
#define FEATURE_IDF_DEFAULT_EVENT_LOOP
#endif

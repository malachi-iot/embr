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

#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
void event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data);
#else
esp_err_t event_handler(void* ctx, system_event_t* event);
#endif

void init_flash();

#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
void wifi_init_sta();
#else
void wifi_init_sta(system_event_cb_t event_handler);
#endif

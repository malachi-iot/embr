#pragma once

#include "esp-helper-platform.h"

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

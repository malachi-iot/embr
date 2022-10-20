#include <esp-helper.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include <driver/gpio.h>

#include <estd/chrono.h>
#include <estd/optional.h>
#include <estd/thread.h>

#include <embr/platform/esp-idf/debounce.hpp>

#include "main.h"
#include "gpio.h"
#include "xthal_clock.h"

extern "C" void app_main()
{
    const char* TAG = "app_main";

    init_flash();
    init_gpio();
    
#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
    wifi_init_sta();
#else
    wifi_init_sta(event_handler);
#endif

    test_polled();
}


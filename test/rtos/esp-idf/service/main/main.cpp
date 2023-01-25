#include <esp-helper.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include <estd/chrono.h>
#include <estd/optional.h>
#include <estd/thread.h>

#include "app.h"
#include "timer.hpp"

extern "C" void app_main()
{
    const char* TAG = "app_main";

    static App app;

    // DEBT: For this example a typical layer1 flavor would be better
    typedef embr::layer0::subject<estd::integral_constant<App*, &app> > subject_type;

    TimerService::runtime<subject_type> timer_service;

    gptimer_config_t config = 
    {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
        .flags = 0
    };

    gptimer_alarm_config_t alarm_config =
    {
        .alarm_count = 1000000, // period = 1s @resolution 1MHz
        .reload_count = 0, // counter will reload with 0 on alarm event
        .flags
        {
            .auto_reload_on_alarm = true, // enable auto-reload
        }
    };

    timer_service._start();
    timer_service.start(&config, &alarm_config);

#if CONFIG_WIFI_ENABLED
#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
    wifi_init_sta();
#else
    wifi_init_sta(event_handler);
#endif
#endif
}


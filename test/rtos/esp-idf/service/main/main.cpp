#include <esp-helper.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include <estd/chrono.h>
#include <estd/optional.h>
#include <estd/thread.h>

#include "app.h"
#include "diagnostic.h"
#include "filter.h"
#include "system.hpp"
#include "timer.hpp"

extern "C" void app_main()
{
    const char* TAG = "app_main";

    static App app;

    // DEBT: Bring in layer1 flavor of subject.  Hopefully with
    // new sparse tuple it will be nearly as efficient as layer0

    // wrap up our App in something ultra-compile time friendly
    typedef estd::integral_constant<App*, &app> app_singleton;

    // create our filter type which reports up to app_singleton
    //typedef TimerFilterService::runtime<
        //embr::layer0::subject<app_singleton> > filter_type;
    typedef embr::layer0::service_type<
        TimerFilterService, app_singleton> filter_type;

    // create a set of observers which the timer will notify
    typedef embr::layer0::subject<Diagnostic, app_singleton, filter_type>
        subject_type;

    // create timer_service with above specified observers
    static TimerService::runtime<subject_type> timer_service;

    typedef estd::integral_constant<decltype(timer_service)*, &timer_service> timer_singleton;
    typedef embr::layer0::subject<Diagnostic, timer_singleton, app_singleton, filter_type>
        subject2_type;

    SystemService::runtime<subject2_type> system_service;

    // For this example a typical layer1 flavor may be better.  Jury is out
    //auto subject = embr::layer1::make_subject(app, filter_type());

    //auto timer_service = embr::make_service<TimerService>(std::move(subject));

#if CONFIG_WIFI_ENABLED
#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
    wifi_init_sta();
#else
    wifi_init_sta(event_handler);
#endif
#endif

    system_service.start();

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

    ESP_LOGI(TAG, "phase 1: timer_service=%p", &timer_service);

    timer_service.start(&config, &alarm_config);

    ESP_LOGI(TAG, "phase 2");

    for(;;)
    {
        static int counter = 0;

        estd::this_thread::sleep_for(estd::chrono::seconds(1));

        ESP_LOGI(TAG, "counting: %d", ++counter);
    }
}


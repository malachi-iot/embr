#include <esp-helper.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include <estd/chrono.h>
#include <estd/optional.h>
#include <estd/thread.h>

#include <embr/platform/esp-idf/service/diagnostic.h>
#include <embr/platform/esp-idf/service/gptimer.hpp>
#include <embr/platform/esp-idf/service/pm.hpp>

using Diagnostic = embr::esp_idf::service::v1::Diagnostic;

#include "app.h"

void App::on_notify(Timer::event::callback e)
{
    ESP_DRAM_LOGI(TAG, "count=%" PRIu64 ", alarm=%" PRIu64,
        e.edata.count_value,
        e.edata.alarm_value);
}


// FIX: Not reaching here - probably a glitch in notify mechanism which can't
// resolve the 'Period' part of this
template <class Period>
void App::on_notify(const TimerFilterService<Period>::event::callback& e)
{
    estd::chrono::milliseconds ms = e.alarm_count();

    ESP_DRAM_LOGI(TAG, "ms=%l", ms.count());
}


extern "C" void app_main()
{
    const char* TAG = "app_main";

    static App app;

    // DEBT: Bring in layer1 flavor of subject.  Hopefully with
    // new sparse tuple it will be nearly as efficient as layer0

    // wrap up our App in something ultra-compile time friendly
    typedef estd::integral_constant<App*, &app> app_singleton;
    typedef embr::layer0::subject<Diagnostic, app_singleton> filter_observer;

    // create our filter type which reports up to app_singleton
    typedef TimerFilterService<estd::micro>::runtime<filter_observer> filter_type;
    //typedef embr::layer0::service_type<
        //TimerFilterService, app_singleton> filter_type;

    // create a set of observers which the timer will notify
    typedef filter_observer::append<filter_type> timer_observer;

    // create timer_service with above specified observers
    static App::Timer::runtime<timer_observer> timer_service;

    typedef estd::integral_constant<decltype(timer_service)*, &timer_service> timer_singleton;
    typedef timer_observer::append<timer_singleton> system_observer;

    embr::esp_idf::service::PowerManager::runtime<system_observer> system_service;

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
        .flags
        {
            .intr_shared = true
        }
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

        if(counter == 10)
        {
            esp_sleep_enable_timer_wakeup(10 * 1000 * 1000);
            system_service.sleep();
        }
    }
}


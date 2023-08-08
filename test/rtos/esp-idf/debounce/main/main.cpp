#include <esp-helper.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"

#include <perfmon.h>

#include <estd/chrono.h>
#include <estd/thread.h>
#include <estd/internal/variadic.h>

#include <embr/platform/esp-idf/gpio.h>

#include <embr/platform/esp-idf/service/diagnostic.h>
#include <embr/platform/esp-idf/service/gptimer.hpp>

using Diagnostic = embr::esp_idf::service::v1::Diagnostic;

#include "app.h"

const char* to_string(embr::debounce::States v)
{
    switch(v)
    {
        case embr::debounce::States::Pressed:    return "pressed";
        case embr::debounce::States::Released:   return "released";
        default:                return "undefined";
    }
}


#define GPIO_INPUT_IO_0     CONFIG_DIAGNOSTIC_GPIO1
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_IO_0)


estd::tuple<
    embr::esp_idf::debounce::v1::Debouncer<GPIO_INPUT_IO_0, true>,
    embr::esp_idf::debounce::v1::Debouncer<4, true> > debouncers;    // DEBT: arbitrary selection of pin 4



inline void App::on_notify(Timer::event::callback)
{
    xtensa_perfmon_start();

    debouncers.visit(embr::esp_idf::debounce::v1::debounce_visitor{}, q);
    //debouncers.visit(debounce_visitor{});

    xtensa_perfmon_stop();
}


static void init_gpio_input()
{
    static const char* TAG = "init_gpio_input";

    ESP_LOGD(TAG, "entry");

    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    ESP_ERROR_CHECK(gpio_config(&io_conf));
}



extern "C" void app_main()
{
    const char* TAG = "app_main";

    static App app;

    // wrap up our App in something ultra-compile time friendly
    typedef estd::integral_constant<App*, &app> app_singleton;
    typedef embr::layer0::subject<Diagnostic, app_singleton> app_observer;

    // create timer_service with above specified observers
    static App::Timer::runtime<app_observer> timer_service;

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
        .alarm_count = 10000, // period = 10ms @resolution 1MHz
        .reload_count = 0, // counter will reload with 0 on alarm event
        .flags
        {
            .auto_reload_on_alarm = true, // enable auto-reload
        }
    };

    ESP_LOGI(TAG, "phase 1: timer_service=%p, sizeof(Event)=%u, sizeof(app.q)=%u, sizeof(debouncers)=%u",
        &timer_service,
        sizeof(embr::debounce::v1::Event),
        sizeof(app.q),
        sizeof(debouncers));

    timer_service.start(&config, &alarm_config);

    init_gpio_input();

    // perfmon stuff lifted from components/perfmon/test/test_perfmon_ansi.c
    // NOTE: At this time I really don't know how to use this feature
    // See https://www2.lauterbach.com/pdf/debugger_xtensa.pdf
    // That doc says to look up "xtensa_debug_guide.pdf" but I can't find that one

    xtensa_perfmon_stop();
    xtensa_perfmon_init(0, 0, 0xffff, 0, 6);
    xtensa_perfmon_reset(0);

    using clock = estd::chrono::freertos_clock;

    clock::time_point last_now = clock::now();
    constexpr estd::chrono::seconds timeout(1);

    for(;;)
    {
        static int counter = 0;

        embr::debounce::v1::Event item;

        if(app.q.receive(&item, estd::chrono::milliseconds(10)))
            ESP_LOGI(TAG, "pin: %u, event: %s (%u)",
                item.pin,
                to_string(item.state),
                (unsigned)item.state);

        if(clock::now() - last_now > timeout)
        {
            last_now += timeout;
            ESP_LOGI(TAG, "counting: %d", ++counter);

            if(counter % 10 == 0)
            {
                xtensa_perfmon_dump();
                xtensa_perfmon_reset(0);
            }
        }
    }
}


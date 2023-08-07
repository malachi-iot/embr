#include <esp-helper.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"

#include <estd/chrono.h>
#include <estd/thread.h>
#include <estd/internal/variadic.h>

#include <embr/platform/esp-idf/gpio.h>

#include <embr/platform/esp-idf/service/diagnostic.h>
#include <embr/platform/esp-idf/service/gptimer.hpp>

#include <embr/internal/debounce/ultimate.h>

using Diagnostic = embr::esp_idf::service::v1::Diagnostic;

#include "app.h"

template <unsigned pin_, bool inverted>
struct Debouncer : embr::internal::DebouncerTracker<uint16_t, inverted>
{
    using base_type = embr::internal::DebouncerTracker<uint16_t, inverted>;

    static constexpr const unsigned pin = pin_;

    bool eval()
    {
        constexpr embr::esp_idf::gpio in((gpio_num_t)pin);

        return base_type::eval(in.level());
    }
};


const char* to_string(DebounceEnum v)
{
    switch(v)
    {
        case BUTTON_PRESSED:    return "pressed";
        case BUTTON_RELEASED:   return "released";
        default:                return "undefined";
    }
}


#define GPIO_INPUT_IO_0     CONFIG_DIAGNOSTIC_GPIO1
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_IO_0)


estd::tuple<
    Debouncer<GPIO_INPUT_IO_0, true>,
    Debouncer<4, true> > debouncers;    // DEBT: arbitrary selection of pin 4

embr::internal::DebouncerTracker<uint16_t, true> d;

using pins = estd::index_sequence<0>;

struct debounce_visitor
{
    static constexpr const char* TAG = "debounce_visitor";

    template <unsigned index, unsigned pin, bool inverted>
    bool operator()(
        estd::variadic::instance<index, Debouncer<pin, inverted> > d)
    {
        // DEBT: Add a -> operator to 'd' for this operation

        if(d.value.eval())
        {
            ESP_DRAM_LOGI(TAG, "on_notify: %s",
                d.value.state() == 1 ? "ON" : "OFF");
        }

        return false;
    }


    template <unsigned index, unsigned pin, bool inverted>
    bool operator()(
        estd::variadic::instance<index, Debouncer<pin, inverted> > d,
        estd::freertos::internal::queue<Item>& q)
    {
        // DEBT: Add a -> operator to 'd' for this operation

        if(d.value.eval())
        {
            auto v = (DebounceEnum) d.value.state();
            ESP_DRAM_LOGV(TAG, "on_notify: %u", v);
            q.send_from_isr(Item{v, pin});
        }

        return false;
    }
};


inline void App::on_notify(Timer::event::callback)
{
    debouncers.visit(debounce_visitor{}, q);
    //debouncers.visit(debounce_visitor{});
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

    // DEBT: Bring in layer1 flavor of subject.  Hopefully with
    // new sparse tuple it will be nearly as efficient as layer0

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

    ESP_LOGI(TAG, "phase 1: timer_service=%p", &timer_service);

    timer_service.start(&config, &alarm_config);

    init_gpio_input();

    for(;;)
    {
        static int counter = 0;

        ESP_LOGI(TAG, "counting: %d", ++counter);

        Item item;

        if(app.q.receive(&item, estd::chrono::seconds(1)))
        {
            ESP_LOGI(TAG, "pin: %u, event: %s", item.pin, to_string(item.state));
        }
    }
}


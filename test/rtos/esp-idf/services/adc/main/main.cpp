#include <esp-helper.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include <estd/chrono.h>
#include <estd/optional.h>
#include <estd/thread.h>

#include <embr/platform/esp-idf/service/diagnostic.h>
#include <embr/platform/esp-idf/service/gpio.hpp>
#include <embr/platform/esp-idf/service/pm.hpp>

using Diagnostic = embr::esp_idf::service::v1::Diagnostic;

#include "app.h"

void App::on_notify(GPIO::event::gpio pin)
{
    ESP_DRAM_LOGD(TAG, "App::on_notify: gpio pin=%u", (unsigned)pin);

    gpio v{pin, (unsigned)gpio_get_level(pin)};

    q.send_from_isr(v);
}

namespace app_domain {

App app;

typedef embr::layer0::subject<Diagnostic, singleton> filter_observer;

App::GPIO::runtime<filter_observer> gpio;
App::ADC::runtime<top_tier> adc;

}


#define GPIO_INPUT_IO_0     CONFIG_DIAGNOSTIC_GPIO1
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_IO_0)


static void init_gpio_input()
{
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    // DEBT: Service is specifically ISR-based GPIO input, reflect that
    // in naming
    app_domain::gpio.start(&io_conf);
}



extern "C" void app_main()
{
    const char* TAG = "app_main";

    init_gpio_input();

    ESP_LOGI(TAG, "startup: GPIO service size=%u %u/%u",
        sizeof(app_domain::gpio),
        sizeof(embr::esp_idf::service::internal::GPIO<false>),
        sizeof(embr::esp_idf::service::internal::GPIOBase));

    for(;;)
    {
        static int counter = 0;
        App::gpio pin;

        while(app_domain::app.q.receive(&pin, estd::chrono::seconds(1)))
            ESP_LOGI(TAG, "pin: %u:%u", (unsigned)pin.pin, pin.level);

        ESP_LOGI(TAG, "counting: %d", ++counter);
    }
}


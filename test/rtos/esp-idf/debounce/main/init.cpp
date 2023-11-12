#include <estd/utility.h>
// DEBT: variadic needs to include utility.h itself
#include <estd/internal/variadic.h>

#include <embr/platform/esp-idf/board.h>
#include <embr/platform/esp-idf/gpio.h>

#include "app.h"

#define GPIO_INPUT_IO_0     CONFIG_DIAGNOSTIC_GPIO1
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_IO_0)

namespace embr { namespace esp_idf {

using status_leds = board_traits::io::where<
        embr::R::traits_selector<
            esp_idf::R::led,
            esp_idf::R::trait::status> >;

}}



// DEBT: Should be a constexpr
embr::esp_idf::gpio status_led((gpio_num_t)-1);


static void init_gptimer()
{
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


static void init_gpio_output()
{
    using leds = embr::esp_idf::status_leds;

    if constexpr (leds::size() > 0)
    {
        // We expect one and only one status LED
        using mux = leds::single;

        status_led = embr::esp_idf::gpio(mux::pin);
        status_led.set_direction(GPIO_MODE_OUTPUT);
    }
}


void App::init()
{
    init_gptimer();
    init_gpio_input();
    init_gpio_output();
}
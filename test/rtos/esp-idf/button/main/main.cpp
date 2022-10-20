#include <esp-helper.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include <driver/gpio.h>

#include <estd/chrono.h>
#include <estd/thread.h>

#include <embr/detail/debounce.hpp>

#include "main.h"
#include "gpio.h"

using namespace embr::detail;
using namespace estd::chrono_literals;

static constexpr gpio_num_t pin = (gpio_num_t)CONFIG_BUTTON_PIN;

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

    Debouncer d;

    for(;;)
    {
        const auto duration = 10ms;
        estd::this_thread::sleep_for(duration);
        bool level = gpio_get_level(pin);
        bool state_changed = d.time_passed(duration, level);

        if(state_changed)
        {
            ESP_LOGI(TAG, "state changed: ");
        }
    }
}


#include <estd/thread.h>

#include "esp_log.h"
#include "esp_system.h"
#include <driver/gpio.h>

#include <embr/platform/esp-idf/debounce.hpp>
#include <embr/platform/esp-idf/timer-scheduler.hpp>

#include "main.h"
#include "gpio.h"
#include "xthal_clock.h"

using namespace embr::detail;
using namespace estd::chrono_literals;

static constexpr gpio_num_t pin = (gpio_num_t)CONFIG_BUTTON_PIN;

void test_isr()
{
    const char* TAG = "test_isr";

#if CONFIG_DRIVER_MODE
    constexpr bool driver_mode = true;
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
#else
    constexpr bool driver_mode = false;
#endif

    embr::esp_idf::Debouncer d(TIMER_GROUP_0, TIMER_0, driver_mode);

    // NOTE: Setting up esp-idf GPIO interrupt characteristics for this pin is handled elsewhere
    // in gpio.cpp
    d.track(CONFIG_BUTTON_PIN);

    int counter = 0;

    for(;;)
    {
        //estd::this_thread::sleep_for(1s);

        embr::esp_idf::Debouncer::Notification n;
        
        if(d.queue.receive(&n, 1s))
        {
            ESP_LOGI(TAG, "state change: %s", to_string(n.state));
        }
        else
            ESP_LOGD(TAG, "counter=%d", ++counter);
    }
}
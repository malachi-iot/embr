#include <esp-helper.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
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
    bool old_level = false;
    auto last_now = estd::chrono::freertos_clock::now();
    int counter = 0;

    esp_task_wdt_add(nullptr);

    // Careful of this situation:
    // https://esp32.com/viewtopic.php?f=2&t=809&start=10
    // Basically, yield won't yield to IDLE task because we are higher priority and
    // sleep_for under 10ms works out to be a yield.  In either case, IDLE task can't
    // feed watchdog and we get a situation
    // DEBT: To get around this, I've disabled watchdog in IDLE tasks.  That is not great.
    // What would be better to is to demote this app_main task priority to below IDLE task priority
    // so that yield() lets IDLE run
    for(;;)
    {
        const auto duration = 1ms;
        // NOTE: Sleep doesn't last NEARLY 1ms!
        // DEBT: May want to put a workaround into sleep_for for FreeRTOS?  Get into why delay
        // is so innacurate
        //estd::this_thread::sleep_for(duration);
        estd::this_thread::yield();

        bool level = gpio_get_level(pin);
        bool state_changed = d.time_passed(duration, level);

        if(state_changed)
        {
            ESP_LOGI(TAG, "state changed: %d", d.state() == Debouncer::On);
        }

        if(old_level != level)
        {
            // Non-debounced, just for diagnostics
            ESP_LOGD(TAG, "level changed: %d", level);
            old_level = level;
        }

        auto now = estd::chrono::freertos_clock::now();

        if(now - last_now > 1s)
        {
            ESP_LOGD(TAG, "counter: %d", ++counter);
            last_now = now;
        }

        esp_task_wdt_reset();
    }
}


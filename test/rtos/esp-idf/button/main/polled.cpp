#include <esp-helper.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include <driver/gpio.h>

#include <estd/chrono.h>
#include <estd/optional.h>
#include <estd/thread.h>

#include <embr/detail/debounce.hpp>

#include "main.h"
#include "gpio.h"
#if defined(__XTENSA__)
#include "xthal_clock.h"
#endif

using namespace embr::detail;
using namespace estd::chrono_literals;

static constexpr gpio_num_t pin = (gpio_num_t)CONFIG_BUTTON_PIN;


void test_polled()
{
    const char* TAG = "test_polled";

    // Full 64-bit resolution counter, way overkill but easy to visualize
    internal::Debouncer<internal::impl::Debouncer<estd::chrono::microseconds> > d1;

    Debouncer d;

    Debouncer d_array[10];

    bool old_level = false;
    int counter = 0, counter2 = 0;
    int dmax = Debouncer::duration::max().count();

    ESP_LOGI(TAG, "start: sizeof(d)=%d, sizeof(d1)=%d, duration::max()=%d", sizeof(d), sizeof(d1), dmax);
    ESP_LOGI(TAG, "start: sizeof(d_array)=%d", sizeof(d_array));

    esp_task_wdt_add(nullptr);
    //vTaskPrioritySet(nullptr, 0);

    auto last_now = estd::chrono::esp_clock::now();
    auto true_duration_start = estd::chrono::esp_clock::now();

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
        // NOTE: Sleep doesn't last NEARLY 1ms!
        // DEBT: May want to put a workaround into sleep_for for FreeRTOS?  Get into why delay
        // is so innacurate
        //const auto duration = 1ms;
        //estd::this_thread::sleep_for(duration);
        estd::this_thread::yield();

        bool level = gpio_get_level(pin);
        auto now = estd::chrono::esp_clock::now();
        estd::chrono::microseconds true_duration = now - true_duration_start;
        true_duration_start = now;
        bool state_changed = d.time_passed(true_duration, level);
        d1.time_passed(true_duration, level);   // dummy call, just to make sure this thing works too

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

        if(now - last_now > 1s)
        {
            esp_task_wdt_reset();

            ESP_LOGD(TAG, "counter: %d, loops: %d, profile: %lldus", ++counter, counter2, true_duration.count());
            last_now = now;
            counter2 = 0;
        }

        ++counter2;
    }
}
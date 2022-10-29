#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/periph_ctrl.h"

#include "scheduler.h"

#include "esp_log.h"

#include <embr/platform/freertos/scheduler.hpp>
#include <embr/platform/esp-idf/gpio.h>

using namespace estd::chrono;
using namespace estd::literals;

embr::scheduler::freertos::Scheduler<5> scheduler;

static constexpr gpio_num_t SLOW_LED_PIN = (gpio_num_t)CONFIG_SLOW_LED_PIN;
static constexpr embr::esp_idf::gpio fast_pin((gpio_num_t)CONFIG_FAST_LED_PIN, GPIO_MODE_OUTPUT);

void gpio_init()
{
    gpio_pad_select_gpio(SLOW_LED_PIN);
    gpio_set_direction(SLOW_LED_PIN, GPIO_MODE_OUTPUT);

    fast_pin.reset();
    fast_pin.set_direction(GPIO_MODE_OUTPUT);
}

void repeater(freertos_clock::time_point* wake, freertos_clock::time_point current)
{

}

void scheduler_init()
{
    static int counter = 0;
    static const char* TAG = "scheduler_init";

    gpio_init();

    scheduler.start();

    typedef FunctorImpl::time_point time_point;

    static int rapid_counter = 0;

    auto rapid_f = FunctorImpl::make_function([](time_point* wake, time_point current)
    {
        // DEBT: Things don't play nice when using long, and it's a mystery why
        int wake_raw = wake->time_since_epoch().count();
        ESP_LOGD(TAG, "rapid_f: %d, wake=%d", rapid_counter, wake_raw);

        fast_pin.level(rapid_counter % 2 == 0);

        if(rapid_counter-- > 0)
        {
            *wake += 250ms;
            wake_raw = wake->time_since_epoch().count();
            ESP_LOGV(TAG, "rapid_f: schedule wakeup=%d", wake_raw);
        }
    });

    auto f = FunctorImpl::make_function([&](time_point* wake, time_point current)
    {
        static bool toggle;

        gpio_set_level(SLOW_LED_PIN, toggle = !toggle);

        // TODO: Add modulo operator to chrono proper

        if(counter % 5 == 0)
        {
            ESP_LOGV(TAG, "Got here");
            rapid_counter = 6;
            // DEBT: Need to do this because otherwise mutex goes into a recursive lock
            auto context = scheduler.create_context(false, false);
            // FIX: This flips out if wake isn't far enough in the future
            scheduler.schedule_with_context(context, *wake + 1ms, rapid_f);
        }

        *wake += 2500ms;
        ESP_LOGI(TAG, "scheduled: counter=%d", counter);
    });

    // Dynamically allocated flavor.  We don't hate this, because it doesn't ever free so it's
    // not a fragmentation risk
    estd::experimental::function<void(time_point*, time_point)> f3(
        [](time_point* wake, time_point current)
        {
            ESP_LOGD(TAG, "f3 counter=%d", counter);
            *wake += 1s;
        });

    scheduler.schedule(freertos_clock::now() + 5s, f);
    scheduler.schedule(freertos_clock::now(), f3);

    for(;;)
    {
        ESP_LOGD(TAG, "counter=%d", ++counter);

        estd::this_thread::sleep_for(1s);
    }
}
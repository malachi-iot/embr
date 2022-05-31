#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/periph_ctrl.h"

#include "scheduler.h"

#include "esp_log.h"

using namespace estd::chrono;
using namespace estd::literals;

embr::experimental::FreeRTOSSchedulerObserver<FreertosFunctorTraits> o;
NotifierObserver o2;
auto s = embr::layer1::make_subject(o, o2);
embr::internal::layer1::Scheduler<FreertosFunctorTraits, 5, decltype(s)> scheduler(s);

static constexpr gpio_num_t FAST_LED_PIN = (gpio_num_t)CONFIG_FAST_LED_PIN;
static constexpr gpio_num_t SLOW_LED_PIN = (gpio_num_t)CONFIG_SLOW_LED_PIN;

void gpio_init()
{
    gpio_pad_select_gpio(FAST_LED_PIN);
    gpio_pad_select_gpio(SLOW_LED_PIN);

    gpio_set_direction(FAST_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(SLOW_LED_PIN, GPIO_MODE_OUTPUT);
}

void scheduler_daemon_init(NotifierObserver& no,
    configSTACK_DEPTH_TYPE usStackDepth = 4096, UBaseType_t uxPriority = 4)
{
    static const char* TAG = "scheduler_daemon_init";

    BaseType_t result = xTaskCreate(scheduler_daemon_task, "embr:scheduler",
                usStackDepth, &scheduler, uxPriority, &no.xSchedulerDaemon);
    if(result != pdPASS)
    {
        ESP_LOGW(TAG, "Could not start scheduler daemon task");
    }
}

void scheduler_init()
{
    static int counter = 0;
    static const char* TAG = "scheduler_init";

    gpio_init();

    scheduler_daemon_init(o2);

    typedef FunctorTraits::time_point time_point;

    static int rapid_counter = 0;

    auto rapid_f = FunctorTraits::make_function([](time_point* wake, time_point current)
    {
        // DEBT: Things don't play nice when using long, and it's a mystery why
        int wake_raw = wake->time_since_epoch().count();
        ESP_LOGD(TAG, "rapid_f: %d, wake=%d", rapid_counter, wake_raw);

        gpio_set_level(FAST_LED_PIN, rapid_counter % 2 == 0);

        if(rapid_counter-- > 0)
        {
            *wake += 250ms;
            wake_raw = wake->time_since_epoch().count();
            ESP_LOGV(TAG, "rapid_f: schedule wakeup=%d", wake_raw);
        }
    });

    auto f = FunctorTraits::make_function([&](time_point* wake, time_point current)
    {
        static bool toggle;

        gpio_set_level(SLOW_LED_PIN, toggle = !toggle);

        // TODO: Add modulo operator to chrono proper

        if(counter % 5 == 0)
        {
            ESP_LOGV(TAG, "Got here");
            rapid_counter = 5;
            scheduler.schedule(*wake + 500ms, rapid_f);
        }

        *wake += 2500ms;
        ESP_LOGI(TAG, "scheduled: counter=%d", counter);
    });

    scheduler.schedule(freertos_clock::now() + 5s, f);
        
    for(;;)
    {
        ESP_LOGD(TAG, "counter=%d", ++counter);

        estd::this_thread::sleep_for(1s);
    }
}
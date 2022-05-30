#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "scheduler.h"

#include "esp_log.h"

using namespace estd::chrono;
using namespace estd::literals;

embr::experimental::FreeRTOSSchedulerObserver<FreertosFunctorTraits> o;
auto s = embr::layer1::make_subject(o);
embr::internal::layer1::Scheduler<FreertosFunctorTraits, 5, decltype(s)> scheduler(s);


void scheduler_init()
{
    static int counter = 0;
    static const char* TAG = "scheduler_init";

    xTaskCreate(scheduler_daemon_task, "embr:scheduler", 
        4096, NULL, 4, NULL);

    typedef FunctorTraits::time_point time_point;

    auto f = FunctorTraits::make_function([](time_point* wake, time_point current)
    {
        *wake += 3000ms;
        ESP_LOGI(TAG, "scheduled: counter=%d", counter);
    });

    scheduler.schedule(freertos_clock::now() + 10s, f);
        
    for(;;)
    {
        ESP_LOGI(TAG, "counter=%d", ++counter);

        estd::this_thread::sleep_for(1s);
    }
}
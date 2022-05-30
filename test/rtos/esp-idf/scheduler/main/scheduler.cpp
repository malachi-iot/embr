#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "scheduler.h"

#include "esp_log.h"

using namespace estd::chrono;
using namespace estd::literals;

embr::experimental::FreeRTOSSchedulerObserver<FreertosFunctorTraits> o;
NotifierObserver o2;
auto s = embr::layer1::make_subject(o, o2);
embr::internal::layer1::Scheduler<FreertosFunctorTraits, 5, decltype(s)> scheduler(s);


void scheduler_init()
{
    static int counter = 0;
    static const char* TAG = "scheduler_init";

    xTaskCreate(scheduler_daemon_task, "embr:scheduler", 
        4096, NULL, 4, NULL);

    typedef FunctorTraits::time_point time_point;

    static int rapid_counter = 0;

    auto rapid_f = FunctorTraits::make_function([](time_point* wake, time_point current)
    {
        ESP_LOGI(TAG, "rapid_f: %d", rapid_counter);

        if(--rapid_counter > 0)
        {
            ESP_LOGD(TAG, "rapid_f: schedule wakeup");
            *wake += 250ms;
        }
    });

    auto f = FunctorTraits::make_function([&](time_point* wake, time_point current)
    {
        // TODO: Add modulo operator to chrono proper

        if(counter % 5 == 0)
        {
            ESP_LOGI(TAG, "Got here");
            rapid_counter = 5;
            scheduler.schedule(*wake + 500ms, rapid_f);
        }

        *wake += 3000ms;
        ESP_LOGI(TAG, "scheduled: counter=%d", counter);
    });

    scheduler.schedule(freertos_clock::now() + 5s, f);
        
    for(;;)
    {
        ESP_LOGI(TAG, "counter=%d", ++counter);

        estd::this_thread::sleep_for(1s);
    }
}
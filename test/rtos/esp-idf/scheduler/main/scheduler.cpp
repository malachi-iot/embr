#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <embr/scheduler.h>
#include <embr/exp/platform/freertos/scheduler.h>

#include <estd/chrono.h>
#include <estd/thread.h>

#include "esp_log.h"

using namespace estd::chrono;
using namespace estd::literals;

using _time_point = freertos_clock::time_point;
using FunctorTraits = embr::internal::experimental::FunctorTraits<freertos_clock::time_point>;

struct FreertosFunctorTraits : FunctorTraits
{
    inline static time_point now()
    { return freertos_clock::now(); }
};

embr::experimental::FreeRTOSSchedulerObserver<FreertosFunctorTraits> o;
auto s = embr::layer1::make_subject(o);
embr::internal::layer1::Scheduler<FreertosFunctorTraits, 5, decltype(s)> scheduler(s);


void scheduler_daemon_task(void*)
{
    for(;;)
    {
        estd::this_thread::sleep_for(milliseconds(100));

        scheduler.process();
    }
}

void scheduler_init()
{
    static int counter = 0;
    static const char* TAG = "scheduler_init";

    xTaskCreate(scheduler_daemon_task, "embr:scheduler", 
        4096, NULL, 4, NULL);

    auto f = FunctorTraits::make_function([](_time_point* wake, _time_point current)
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
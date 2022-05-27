#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <embr/scheduler.h>
#include <embr/exp/platform/freertos/scheduler.h>

#include <estd/chrono.h>
#include <estd/thread.h>

#include "esp_log.h"

using namespace estd::chrono;

void scheduler_daemon_task(void*)
{
    for(;;)
    {
        estd::this_thread::sleep_for(milliseconds(100));
    }
}

void scheduler_init()
{
    const char* TAG = "scheduler_init";

    xTaskCreate(scheduler_daemon_task, "embr:scheduler", 
        4096, NULL, 4, NULL);

    for(;;)
    {
        static int counter = 0;

        ESP_LOGI(TAG, "counter=%d", ++counter);
        
        estd::this_thread::sleep_for(seconds(1));
    }
}
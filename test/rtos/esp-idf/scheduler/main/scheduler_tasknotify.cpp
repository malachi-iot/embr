#include <estd/chrono.h>
#include <estd/thread.h>

#include "scheduler.h"

using namespace estd::chrono;
using namespace estd::literals;

#if SCHEDULER_APPROACH == SCHEDULER_APPROACH_TASKNOTIFY
void scheduler_daemon_task(void*)
{
    o2.xSchedulerDaemon = xTaskGetCurrentTaskHandle();

    const char* TAG = "scheduler_daemon_task";

//    typedef FunctorTraits::time_point time_point;

    ESP_LOGI(TAG, "start");

    for(;;)
    {
        auto duration = scheduler.top_time() - freertos_clock::now();
        uint32_t r = ulTaskNotifyTake(pdTRUE, duration.count());

        ESP_LOGD(TAG, "wake: %u", r);

        scheduler.process();
    }
}
#endif
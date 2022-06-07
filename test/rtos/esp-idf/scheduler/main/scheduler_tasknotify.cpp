#include <estd/chrono.h>
#include <estd/thread.h>

#include "scheduler.h"

using namespace estd::chrono;
using namespace estd::literals;


#if SCHEDULER_APPROACH == SCHEDULER_APPROACH_TASKNOTIFY
void scheduler_daemon_task(void*)
{
    const char* TAG = "scheduler_daemon_task";

//    typedef FunctorTraits::time_point time_point;

    ESP_LOGI(TAG, "start");

    for(;;)
    {
        scheduler.process();

        freertos_clock::duration duration = scheduler.top_time() - freertos_clock::now();

        // DEBT: ESP_LOGV speed will actually slow things down 
        // so we double-calculate duration.  Instead, we should
        // ifdef and only double-do it when verbose level is specified
        // DEBT: We really need to activate FEATURE_ESTD_CHRONO_LOWPRECISION
        ESP_LOGV(TAG, "waiting for %lld ticks", duration.count());

        duration = scheduler.top_time() - freertos_clock::now();

        if(duration.count() > 0)
        {
            uint32_t r = ulTaskNotifyTake(pdTRUE, duration.count());

            ESP_LOGD(TAG, "wake: %u", r);
        }
    }
}
#endif
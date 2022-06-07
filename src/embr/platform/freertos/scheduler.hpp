#pragma once

#include "../../exp/platform/freertos/scheduler.h"

#include <estd/chrono.h>
#include <estd/thread.h>    // Brings in FreeRTOS task notify and friends

#ifdef ESP_PLATFORM
#include "esp_log.h"
#endif

namespace embr { namespace freertos {

// NOTE: Will never actually be inline.  Specified as such to suppress compiler
// warnings about unused static function
inline static void scheduler_bruteforce_daemon_task(void* data)
{
    for(;;)
    {
        scheduler.process();

        vTaskDelay(1);
    }
}

template <class TScheduler>
static void scheduler_notify_daemon_task(void* data)
{
    using namespace estd::chrono;

    const char* TAG = "scheduler_notify_daemon_task";
    TScheduler& scheduler = *reinterpret_cast<TScheduler*>(data);

#ifdef ESP_PLATFORM
    ESP_LOGI(TAG, "start");
#endif

    for(;;)
    {
        scheduler.process();

        freertos_clock::duration duration = scheduler.top_time() - freertos_clock::now();

#ifdef ESP_PLATFORM
#if CONFIG_LOG_MAXIMUM_LEVEL == ESP_LOG_VERBOSE
        // DEBT: We really need to activate FEATURE_ESTD_CHRONO_LOWPRECISION
        ESP_LOGV(TAG, "waiting for %lld ticks", duration.count());

        duration = scheduler.top_time() - freertos_clock::now();
#endif
#endif

        // Only sleep if there's actually an interval we need to wait.
        // It's an acceptable use case for count() <= 0 since this task
        // may wake up too slowly, or someone can schedule for "right now"
        // which even the fastest wakeup time has already passed (in theory)
        if(duration.count() > 0)
        {
            uint32_t r = ulTaskNotifyTake(pdTRUE, duration.count());

#ifdef ESP_PLATFORM
            ESP_LOGD(TAG, "wake: %u", r);
#endif
        }
    }
}


template <class TScheduler>
BaseType_t scheduler_notify_daemon_init(
    TScheduler& scheduler,
    embr::freertos::experimental::NotifierObserver& no,
#ifdef ESP_PLATFORM
    configSTACK_DEPTH_TYPE usStackDepth = CONFIG_EMBR_SCHEDULER_TASKSIZE,
    UBaseType_t uxPriority = CONFIG_EMBR_SCHEDULER_PRIORITY)
#else
    configSTACK_DEPTH_TYPE usStackDepth = 4096, UBaseType_t uxPriority = 4)
#endif
{
    static const char* TAG = "scheduler_notify_daemon_init";

    BaseType_t result = xTaskCreate(scheduler_notify_daemon_task<TScheduler>, "embr:scheduler",
                                    usStackDepth, &scheduler, uxPriority, &no.xSchedulerDaemon);
    if(result != pdPASS)
    {
#ifdef ESP_PLATFORM
        ESP_LOGW(TAG, "Could not start scheduler daemon task");
#endif
    }

    return result;
}



}}
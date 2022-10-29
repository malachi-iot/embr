#pragma once

#include "../../exp/platform/freertos/scheduler.h"

#include <estd/chrono.h>
#include <estd/thread.h>    // Brings in FreeRTOS task notify and friends

#ifdef ESP_PLATFORM
#include "esp_log.h"
#endif

// We call this embr::scheduler::freertos and not embr::freertos::scheduler because
// our scheduler operates in its own specific way, different than FreeRTOS'
// scheduler.  embr::freertos::scheduler feels more sensible for a wrapper, in theory
namespace embr { namespace scheduler { namespace freertos {

// NOTE: Will never actually be inline.  Specified as such to suppress compiler
// warnings about unused static function
template <class TScheduler>
inline static void bruteforce_daemon_task(void* data)
{
    TScheduler& scheduler = *reinterpret_cast<TScheduler*>(data);

    for(;;)
    {
        scheduler.process();

        vTaskDelay(1);
    }
}

template <class TScheduler>
static void notify_daemon_task(void* data)
{
    using namespace estd::chrono;

    TScheduler& scheduler = *reinterpret_cast<TScheduler*>(data);

#ifdef ESP_PLATFORM
    static constexpr const char* TAG = "scheduler_notify_daemon_task";
    ESP_LOGI(TAG, "start");
#endif

    // DEBT: We need to account for WDT here, probably cap the value fed into ulTaskNotifyTake

    for(;;)
    {
        scheduler.process();

        freertos_clock::duration duration;
        
        // If nothing's scheduled at all, wait for notification for a while
        if(!scheduler.empty())
        {
            duration = scheduler.top_time() - freertos_clock::now();

#ifdef ESP_PLATFORM
#if CONFIG_LOG_MAXIMUM_LEVEL == ESP_LOG_VERBOSE
            // DEBT: We really need to activate FEATURE_ESTD_CHRONO_LOWPRECISION
            ESP_LOGV(TAG, "waiting for %lld ticks", duration.count());

            duration = scheduler.top_time() - freertos_clock::now();
#endif
#endif
        }
        else
        {
            duration = estd::chrono::seconds(2);
        }

        // Only sleep if there's actually an interval we need to wait.
        // It's an acceptable use case for count() <= 0 since this task
        // may wake up too slowly, or someone can schedule for "right now"
        // which even the fastest wakeup time has already passed (in theory)
        if(duration.count() > 0)
        {
            // DEBT: Keep eye out for race condition where notification was sent
            // after above empty check but before we reach here.  Not sure if notification
            // queues up (I think it does) - if so, we are OK
            uint32_t r = ulTaskNotifyTake(pdTRUE, duration.count());

#ifdef ESP_PLATFORM
            ESP_LOGD(TAG, "wake: %u", r);
#endif
        }
    }
}


template <class TScheduler>
inline BaseType_t notify_daemon_init(
    TScheduler& scheduler,
    embr::freertos::experimental::NotifierObserver& no,
#ifdef ESP_PLATFORM
    configSTACK_DEPTH_TYPE usStackDepth = CONFIG_EMBR_FREERTOS_SCHEDULER_TASKSIZE,
    UBaseType_t uxPriority = CONFIG_EMBR_FREERTOS_SCHEDULER_PRIORITY)
#else
    configSTACK_DEPTH_TYPE usStackDepth = 4096, UBaseType_t uxPriority = 4)
#endif
{
    static const char* TAG = "scheduler_notify_daemon_init";

    BaseType_t result = xTaskCreate(notify_daemon_task<TScheduler>, "embr:scheduler",
                                    usStackDepth, &scheduler, uxPriority, &no.xSchedulerDaemon);
    if(result != pdPASS)
    {
#ifdef ESP_PLATFORM
        ESP_LOGW(TAG, "Could not start scheduler daemon task");
#endif
    }

    return result;
}



template <class TScheduler>
inline BaseType_t bruteforce_daemon_init(
    TScheduler& scheduler,
#ifdef ESP_PLATFORM
    configSTACK_DEPTH_TYPE usStackDepth = CONFIG_EMBR_FREERTOS_SCHEDULER_TASKSIZE,
    UBaseType_t uxPriority = CONFIG_EMBR_FREERTOS_SCHEDULER_PRIORITY)
#else
    configSTACK_DEPTH_TYPE usStackDepth = 4096, UBaseType_t uxPriority = 4)
#endif
{
    static const char* TAG = "scheduler_bruteforce_daemon_init";

    BaseType_t result = xTaskCreate(bruteforce_daemon_task<TScheduler>, "embr:scheduler",
                                    usStackDepth, &scheduler, uxPriority, nullptr);
    if(result != pdPASS)
    {
#ifdef ESP_PLATFORM
        ESP_LOGW(TAG, "Could not start scheduler daemon task");
#endif
    }

    return result;
}


template <unsigned N, class ...TObservers>
class Scheduler : public internal::layer1::Scheduler<
    N,
    embr::freertos::experimental::FunctorImpl,
    embr::layer1::subject<embr::freertos::experimental::NotifierObserver, TObservers...>
    >
{
    typedef internal::layer1::Scheduler<
        N,
        embr::freertos::experimental::FunctorImpl,
        embr::layer1::subject<embr::freertos::experimental::NotifierObserver, TObservers...> >
        base_type;

    embr::freertos::experimental::NotifierObserver& notifier_observer()
    {
        return base_type::subject_provider::value().template get<0>();
    }

public:
    /// Starts a notifier-based scheduler daemon task
    void start()
    {
        notify_daemon_init(*this, notifier_observer());
    }


    void stop()
    {
#ifdef INCLUDE_vTaskDelete
        vTaskDelete(notifier_observer().xSchedulerDaemon);
#endif
    }
};


}}}
#pragma once

#include <estd/chrono.h>
#include <estd/port/freertos/mutex.h>
#include <estd/port/freertos/thread.h>

#include <embr/scheduler.hpp>
#include <embr/platform/freertos/scheduler.h>

#ifdef ESP_PLATFORM
#include "esp_log.h"
#endif

namespace embr { namespace scheduler { namespace freertos {

template <class TScheduler>
void notify_daemon_task(void* data);

}}}

namespace embr { namespace freertos { namespace experimental {

/// Dummy/no-op observer
/// @tparam TTraits
template <class TImpl>
struct SchedulerObserver
{
    typedef TImpl traits_type;

    void on_notify(embr::internal::events::Processing<traits_type>)
    {
    }

    void on_notify(embr::internal::events::Scheduling<traits_type>)
    {
    }

    void on_notify(embr::internal::events::Scheduled<traits_type>)
    {
    }

    void on_notify(embr::internal::events::Removed<traits_type>)
    {
    }
};


// DEBT: Break this out a bit the way esp32 timer mode was where we pass in control_structure
// but definitely lean heavily towards *default* being FunctorImpl - so that we can do FreeRTOS
// stuff without: 1) being bound to functors and 2) using simpler on_scheduled vs. subject/observer pattern
struct FunctorImpl :
    embr::internal::scheduler::impl::Function<estd::chrono::freertos_clock::time_point>
{
    typedef embr::scheduler::freertos::context_factory context_factory;
    typedef embr::scheduler::freertos::context_type context_type;
    typedef embr::scheduler::freertos::timed_mutex<false, true> mutex;
    typedef embr::scheduler::freertos::timed_mutex<true, true> binary_semaphore;

    inline static time_point now()
    { return estd::chrono::freertos_clock::now(); }
};



// DEBT: Only bracketed out due to logging.  Consider using
// https://www.freertos.org/logging.html
#ifdef ESP_PLATFORM
// Specifically, FreeRTOS Task Notifier -
// not relevant for brute force polled approach
struct NotifierObserver
{
    static constexpr const char* TAG = "NotifyObserver";

    TaskHandle_t xSchedulerDaemon;
    // NOTE: Would consider time_point or similar here instead but a flag is equally efficient
    // and puts less demand on templating.  Also we expect schedule operations to be atomic, so
    // by convention one early_wakeup flag is sufficient.
    bool early_wakeup;

    NotifierObserver() :
        // DEBT: Optimization, would be nice to pass this in and also
        // have a 'strict' flag of sorts to optimize out boundary checks
        xSchedulerDaemon(NULLPTR),
        early_wakeup(false) {}

    //template <class TScheduler>
    //void on_notify(embr::internal::events::Scheduling<FreertosFunctorTraits> scheduling, TScheduler& scheduler)
    
    //template <class TContainer, class TImpl, class TSubject>
    //void on_notify(embr::internal::events::Scheduling<TImpl> scheduling,
    //    embr::internal::Scheduler<TContainer, TImpl, TSubject>& scheduler)

    template <class TScheduler>
    //void on_notify(embr::internal::events::Scheduling<typename TScheduler::impl_type> scheduling,
    void on_notify(typename TScheduler::event::scheduling scheduling,
        embr::internal::SchedulerContextBase<TScheduler>& context)
    {
        if((early_wakeup = scheduling.value.wake < context.scheduler().top_time()))
        {
            ESP_LOGV(TAG, "on_notify(scheduling) early wakeup tagged");
        }

        ESP_LOGV(TAG, "on_notify(scheduling)");
    }

    // 'bigger' one above consumes call, so this one doesn't get called.  Acceptable behavior
    template <class TImpl>
    void on_notify(embr::internal::events::Scheduling<TImpl> scheduling)
    {
        // Doing as a warning because we shouldn't see this ever, though
        // it may be benign if we do
        ESP_LOGW(TAG, "on_notify(scheduling) 2");
    }

    template <class Impl>
    void on_notify(embr::internal::events::Scheduled<Impl> scheduled)
    {
        if(early_wakeup)
        {
            // NOTE: Only doing warning temporarily as we build this out
            ESP_LOGD(TAG, "on_notify(scheduled) early wakeup");

            if(xSchedulerDaemon == NULLPTR)
            {
                ESP_LOGE(TAG, "on_notify(scheduled) failure - daemon not running, aborting wakeup");
                return;
            }
                
            // This will result in immediately waking up daemon, which we expect to turn right around
            // and sleep again - but for a shorter period of time.  Therefore, two wakes occur.
            xTaskNotifyGive(xSchedulerDaemon);
        }

        ESP_LOGV(TAG, "on_notify(scheduled)");
    }
};
#endif

struct FunctorImpl2 : FunctorImpl
{
    static constexpr const char* TAG = "FunctorImpl2";

    estd::freertos::wrapper::task daemon;
    bool early_wakeup;

    FunctorImpl2() :
        daemon(NULLPTR),
        early_wakeup(false) {}

    template <class TScheduler>
    void start(TScheduler* scheduler,
        configSTACK_DEPTH_TYPE usStackDepth = CONFIG_EMBR_FREERTOS_SCHEDULER_TASKSIZE,
        UBaseType_t uxPriority = CONFIG_EMBR_FREERTOS_SCHEDULER_PRIORITY)
    {
        TaskHandle_t t;
        BaseType_t result = xTaskCreate(embr::scheduler::freertos::notify_daemon_task<TScheduler>, "embr:scheduler2",
                                        usStackDepth, scheduler, uxPriority, &t);

        if(result != pdPASS)
        {
#ifdef ESP_PLATFORM
            ESP_LOGW(TAG, "Could not start scheduler daemon task");
#endif
        }

        daemon = t;
    }

    void stop()
    {
#ifdef INCLUDE_vTaskDelete
        vTaskDelete(daemon);
#endif
    }

    template <class T, class TScheduler>
    void on_scheduling(T& t, embr::internal::SchedulerContextBase<TScheduler>& context)
    {
        if((early_wakeup = get_time_point(t) < context.scheduler().top_time()))
        {
            ESP_LOGV(TAG, "on_notify(scheduling) early wakeup tagged");
        }

        ESP_LOGD(TAG, "on_notify(scheduling)");
    }

    template <class T, class TScheduler>
    void on_scheduled(T&, embr::internal::SchedulerContextBase<TScheduler>& context)
    {
        if(early_wakeup)
        {
            // NOTE: Only doing warning temporarily as we build this out
            ESP_LOGD(TAG, "on_notify(scheduled) early wakeup");

            if(daemon == NULLPTR)
            {
                ESP_LOGE(TAG, "on_notify(scheduled) failure - daemon not running, aborting wakeup");
                return;
            }
                
            // This will result in immediately waking up daemon, which we expect to turn right around
            // and sleep again - but for a shorter period of time.  Therefore, two wakes occur.
            daemon.notify_give();
        }

        ESP_LOGV(TAG, "on_notify(scheduled)");
    }
};

}}}
#pragma once

#include <estd/chrono.h>
#include <estd/port/freertos/mutex.h>

#include <embr/scheduler.h>

#ifdef ESP_PLATFORM
#include "esp_log.h"
#endif

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


struct FunctorImpl :
    embr::internal::scheduler::impl::Function<estd::chrono::freertos_clock::time_point>
{
    // TODO: Change namespace to estd::freertos
    typedef estd::experimental::mutex mutex;

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

    NotifierObserver() : early_wakeup(false) {}

    //template <class TScheduler>
    //void on_notify(embr::internal::events::Scheduling<FreertosFunctorTraits> scheduling, TScheduler& scheduler)
    template <class TContainer, class TImpl, class TSubject>
    void on_notify(embr::internal::events::Scheduling<TImpl> scheduling,
        embr::internal::Scheduler<TContainer, TImpl, TSubject>& scheduler)
    {
        if((early_wakeup = scheduling.value.wake < scheduler.top_time()))
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

    template <class TImpl>
    void on_notify(embr::internal::events::Scheduled<TImpl> scheduled)
    {
        if(early_wakeup)
        {
            // NOTE: Only doing warning temporarily as we build this out
            ESP_LOGD(TAG, "on_notify(scheduled) early wakeup");

            // This will result in immediately waking up daemon, which we expect to turn right around
            // and sleep again - but for a shorter period of time.  Therefore, two wakes occur.
            xTaskNotifyGive(xSchedulerDaemon);
        }

        ESP_LOGV(TAG, "on_notify(scheduled)");
    }
};
#endif



}}}
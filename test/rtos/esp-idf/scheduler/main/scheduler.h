#pragma once

#include <embr/scheduler.h>
#include <embr/exp/platform/freertos/scheduler.h>

#include <estd/chrono.h>
#include <estd/thread.h>

#include "esp_log.h"


#define SCHEDULER_APPROACH_BRUTEFORCE 1
#define SCHEDULER_APPROACH_TASKNOTIFY 2

#ifndef SCHEDULER_APPROACH
#define SCHEDULER_APPROACH SCHEDULER_APPROACH_TASKNOTIFY
#endif


void scheduler_daemon_task(void*);


using FunctorTraits = embr::internal::experimental::FunctorTraits<
    estd::chrono::freertos_clock::time_point>;

struct FreertosFunctorTraits : FunctorTraits
{
    inline static time_point now()
    { return estd::chrono::freertos_clock::now(); }
};

// Specifically, FreeRTOS Task Notifier
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
    void on_notify(embr::internal::events::Scheduling<FreertosFunctorTraits> scheduling,
        embr::internal::Scheduler<TContainer, TImpl, TSubject>& scheduler)
    {
        if((early_wakeup = scheduling.value.wake < scheduler.top_time()))
        {
            ESP_LOGD(TAG, "on_notify(scheduling) early wakeup tagged");
        }

        ESP_LOGI(TAG, "on_notify(scheduling) 1");
    }

    // 'bigger' one above consumes call, so this one doesn't get called.  Acceptable behavior
    void on_notify(embr::internal::events::Scheduling<FreertosFunctorTraits> scheduling)
    {
        ESP_LOGI(TAG, "on_notify(scheduling) 2");
    }

    void on_notify(embr::internal::events::Scheduled<FreertosFunctorTraits> scheduled)
    {
        if(early_wakeup)
            xTaskNotifyGive(xSchedulerDaemon);

        ESP_LOGI(TAG, "on_notify(scheduled)");
    }
};


extern embr::experimental::FreeRTOSSchedulerObserver<FreertosFunctorTraits> o;
extern NotifierObserver o2;
extern embr::internal::layer1::Scheduler<FreertosFunctorTraits, 5, 
    decltype(embr::layer1::make_subject(o, o2))> scheduler;


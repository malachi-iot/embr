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

    template <class TScheduler>
    void on_notify(embr::internal::events::Scheduling<FreertosFunctorTraits> scheduling, TScheduler* scheduler)
    {
        //if(scheduled.value.wake <
        ESP_LOGI(TAG, "on_notify(scheduling)");
    }

    void on_notify(embr::internal::events::Scheduled<FreertosFunctorTraits> scheduled)
    {
        ESP_LOGI(TAG, "on_notify(scheduled)");
    }
};


extern embr::experimental::FreeRTOSSchedulerObserver<FreertosFunctorTraits> o;
extern NotifierObserver o2;
extern embr::internal::layer1::Scheduler<FreertosFunctorTraits, 5, 
    decltype(embr::layer1::make_subject(o, o2))> scheduler;


#pragma once

#include <embr/scheduler.h>
#include <embr/exp/platform/freertos/scheduler.h>

#include <estd/chrono.h>
#include <estd/thread.h>


#define SCHEDULER_APPROACH_BRUTEFORCE 1
#define SCHEDULER_APPROACH_TASKNOTIFY 2

#ifndef SCHEDULER_APPROACH
#define SCHEDULER_APPROACH SCHEDULER_APPROACH_BRUTEFORCE
#endif


void scheduler_daemon_task(void*);


using FunctorTraits = embr::internal::experimental::FunctorTraits<
    estd::chrono::freertos_clock::time_point>;

struct FreertosFunctorTraits : FunctorTraits
{
    inline static time_point now()
    { return estd::chrono::freertos_clock::now(); }
};


extern embr::experimental::FreeRTOSSchedulerObserver<FreertosFunctorTraits> o;
extern embr::internal::layer1::Scheduler<FreertosFunctorTraits, 5, 
    decltype(embr::layer1::make_subject(o))> scheduler;


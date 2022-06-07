#pragma once

#include <embr/scheduler.h>
#include <embr/exp/platform/freertos/scheduler.h>

#include <estd/chrono.h>
#include <estd/thread.h>

#include "esp_log.h"


#define SCHEDULER_APPROACH_BRUTEFORCE 1
#define SCHEDULER_APPROACH_TASKNOTIFY 2

#ifndef SCHEDULER_APPROACH
#define SCHEDULER_APPROACH 2
#endif


void scheduler_daemon_task(void*);


using FunctorTraits = embr::internal::experimental::FunctorTraits<
    estd::chrono::freertos_clock::time_point>;
using FreertosFunctorTraits = embr::freertos::experimental::FunctorTraits;
using NotifierObserver = embr::freertos::experimental::NotifierObserver;

extern embr::freertos::experimental::SchedulerObserver<FreertosFunctorTraits> o;
extern embr::freertos::experimental::NotifierObserver o2;
extern embr::internal::layer1::Scheduler<FreertosFunctorTraits, 5, 
    decltype(embr::layer1::make_subject(o, o2))> scheduler;


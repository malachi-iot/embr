#pragma once

#define SCHEDULER_APPROACH_BRUTEFORCE 1
#define SCHEDULER_APPROACH_TASKNOTIFY 2

#ifndef SCHEDULER_APPROACH
#define SCHEDULER_APPROACH 2
#endif

#define SCHEDULE_MANUAL_INIT 0


#include <embr/scheduler.h>
#include <embr/exp/platform/freertos/scheduler.h>

#include <estd/chrono.h>
#include <estd/thread.h>

#include "esp_log.h"



using FunctorImpl = embr::freertos::experimental::FunctorImpl;

// NOTE: All these externs are for posterity, or in case we feel like
// adding more modules later
#if SCHEDULE_MANUAL_INIT
using NotifierObserver = embr::freertos::experimental::NotifierObserver;

extern embr::freertos::experimental::SchedulerObserver<FunctorImpl> o;
#if SCHEDULER_APPROACH == SCHEDULER_APPROACH_TASKNOTIFY
extern embr::freertos::experimental::NotifierObserver o2;
typedef decltype(embr::layer1::make_subject(o, o2)) subject_type;
#else
typedef decltype(embr::layer1::make_subject(o)) subject_type;
#endif
extern embr::internal::layer1::Scheduler<FunctorImpl, 5, subject_type> scheduler;
#endif

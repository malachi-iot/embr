#pragma once

#define SCHEDULER_APPROACH_BRUTEFORCE 1
#define SCHEDULER_APPROACH_TASKNOTIFY 2

#ifndef SCHEDULER_APPROACH
#define SCHEDULER_APPROACH 2
#endif

#include <embr/scheduler.h>
#include <embr/exp/platform/freertos/scheduler.h>

#include <estd/chrono.h>
#include <estd/thread.h>

#include "esp_log.h"



using FunctorImpl = embr::freertos::experimental::FunctorImpl;


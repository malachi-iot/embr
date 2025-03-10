#pragma once

#include <embr/profiler.h>

using clock_type = estd::chrono::esp_clock;

void test_dsp();

using Profiler = embr::Profiler<clock_type>;
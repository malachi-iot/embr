#pragma once

#include <embr/profiler.h>

using clock_type = estd::chrono::esp_clock;
using duration = typename clock_type::duration;

void test_dsp();
void test_span();

inline namespace perf {

using Profiler = embr::Profiler<clock_type>;

}

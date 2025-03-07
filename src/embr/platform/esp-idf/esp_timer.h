// https://docs.espressif.com/projects/esp-idf/en/v4.4.2/esp32/api-reference/system/esp_timer.html
#pragma once

#include <esp_timer.h>

namespace embr { namespace esp_idf {

class esp_timer
{

};

namespace clock {

// Special case of pseudo estd::chrono::clock type, returning raw values instead of standard
// time_point/duration.  Useful for scenarios where:
// 1. Extra concerned that optimizer won't remove c++ layers
// 2. Direct reference to uint64_t us is convenient without .count() call
// Designed to be used in conjunction with embr::Profiler
struct esp_timer
{
    using rep = estd::chrono::internal::micro_rep;
    //typedef micro period;
    using time_point = rep;
    using duration = rep;
    
    static time_point now()
    {
        return time_point(duration(esp_timer_get_time()));
    }
};

}

}}
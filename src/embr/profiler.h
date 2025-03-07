#pragma once

#if ESP_PLATFORM
#include "platform/esp-idf/esp_timer.h"
#endif

namespace embr {

template <class Clock>
class Profiler
{
public:
    using clock_type = Clock;
    using time_point = typename clock_type::time_point;
    using duration = typename clock_type::duration;

private:
    time_point start_;

public:
    Profiler() : start_(clock_type::now())
    {

    }

    constexpr time_point start() const { return start_; } 

    duration mark()
    {
        const time_point now = clock_type::now();
        const duration elapsed = now - start_;
        start_ = now;
        return elapsed;
    }

    void reset()
    {
        start_ = clock_type::now();
    }
};


}
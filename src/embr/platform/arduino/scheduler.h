// EXPERIMENTAL
#include <Arduino.h>

#include <estd/port/arduino/chrono.h>

#include "../../scheduler.h"

namespace embr {

namespace experimental {

typedef decltype(millis()) arduino_time_point;

struct ArduinoSchedulerTraits : 
    embr::internal::scheduler::impl::Function<arduino_time_point>
{
    static arduino_time_point now() { return millis(); }
};

struct ArduinoChronoSchedulerTraits : 
    embr::internal::scheduler::impl::Function<arduino_time_point>
{
    static estd::chrono::arduino_clock::time_point
        now() { return estd::chrono::arduino_clock::now(); }
};

}

}
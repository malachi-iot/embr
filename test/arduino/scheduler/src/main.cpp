// EXPERIMENTAL

#include <Arduino.h>

#include <estd/string.h>
#include <estd/chrono.h>
#include <estd/queue.h>
#include <estd/thread.h>

#include <embr/observer.h>
#include <embr/platform/arduino/scheduler.h>

using FunctorTraits = embr::experimental::ArduinoSchedulerTraits;
using time_point = FunctorTraits::time_point;

CONSTEXPR int LED_PIN = LED_BUILTIN;

embr::internal::layer1::Scheduler<FunctorTraits, 5> scheduler;

void setup()
{
    static bool on = false;
    static auto f = FunctorTraits::make_function([](time_point* wake, time_point current)
    {
        digitalWrite(LED_PIN, on ? LOW : HIGH);
        on = !on;
        *wake += 500;
    });

    scheduler.schedule_now(f);
   
    pinMode(LED_PIN, OUTPUT);
}


void loop()
{
    scheduler.process();
}
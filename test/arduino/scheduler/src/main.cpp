// EXPERIMENTAL

#include <Arduino.h>

#include <estd/string.h>
#include <estd/chrono.h>
#include <estd/queue.h>
#include <estd/thread.h>

// DEBT: Not sure how I'm feeling about it.  I removed steady clock alias a little
// while ago from estd in this context.  Do I miss it?
namespace estd { namespace chrono {

using steady_clock = arduino_clock;

}}

#include <embr/observer.h>
#include <embr/platform/arduino/scheduler.h>
#include <embr/scheduler.hpp>

using FunctorTraits = embr::experimental::ArduinoSchedulerTraits;
using time_point = FunctorTraits::time_point;

CONSTEXPR int LED_PIN = LED_BUILTIN;

embr::internal::layer1::Scheduler<5, FunctorTraits> scheduler;

#ifndef ENABLE_SERIAL
#define ENABLE_SERIAL 1
#endif

void setup()
{
#if ENABLE_SERIAL
    Serial.begin(9600);
#endif

    static bool on = false;
    static auto f = FunctorTraits::make_function([](time_point* wake, time_point current)
    {
        digitalWrite(LED_PIN, on ? LOW : HIGH);
        on = !on;
        *wake += 500;
    });

#if ENABLE_SERIAL
    static auto f2 = FunctorTraits::make_function([](time_point* wake, time_point current)
    {
        static int counter = 0;
        *wake += 5000;
        Serial.println(++counter);
    });
#endif

    scheduler.schedule_now(f);
#if ENABLE_SERIAL
    scheduler.schedule_now(f2);
#endif   
    pinMode(LED_PIN, OUTPUT);
}


void loop()
{
    scheduler.process();
}
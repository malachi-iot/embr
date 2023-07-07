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
using function_type = FunctorTraits::function_type;

CONSTEXPR int LED_PIN = LED_BUILTIN;

embr::internal::layer1::Scheduler<5, FunctorTraits> scheduler;

#ifndef ENABLE_SERIAL
#define ENABLE_SERIAL 1
#endif

// On attiny85 saves 2 bytes
#ifndef ENABLE_DIRECT_MODEL
#define ENABLE_DIRECT_MODEL 0
#endif

void setup()
{
#if ENABLE_SERIAL
    Serial.begin(9600);
#endif

    static bool on = false;
#if ENABLE_DIRECT_MODEL
    static auto m = function_type::make_model([](time_point* wake, time_point current)
#else
    static auto f = FunctorTraits::make_function([](time_point* wake, time_point current)
#endif
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

#if ENABLE_DIRECT_MODEL
    scheduler.schedule_now(&m);
#else
    scheduler.schedule_now(f);
#endif
#if ENABLE_SERIAL
    scheduler.schedule_now(f2);
#endif   
    pinMode(LED_PIN, OUTPUT);
}


void loop()
{
    scheduler.process();
}
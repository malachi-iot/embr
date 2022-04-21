// EXPERIMENTAL

#include <Arduino.h>

#include <estd/string.h>
#include <estd/chrono.h>
#include <estd/queue.h>
#include <estd/thread.h>

#include <embr/observer.h>
#include <embr/scheduler.h>

using FunctorTraits = embr::internal::experimental::FunctorTraits<decltype(millis())>;
using time_point = FunctorTraits::time_point;

CONSTEXPR int LED_PIN = LED_BUILTIN;

embr::internal::layer1::Scheduler<FunctorTraits, 5> scheduler;

void setup()
{
    static auto f_on = FunctorTraits::make_function([](time_point* wake, time_point current)
    {
        digitalWrite(LED_PIN, LOW);
        *wake += 2000;
    });

    static auto f_off = FunctorTraits::make_function([](time_point* wake, time_point current)
    {
        digitalWrite(LED_PIN, HIGH);
        *wake += 2000;
    });

    static auto dummy = FunctorTraits::make_function([](time_point* wake, time_point current)
    {
        *wake += 100;
    });

    time_point start = millis() + 1000;

    //scheduler.schedule(start, f_on);
    //scheduler.schedule(start + 1000, f_off);
    //scheduler.schedule(start, dummy);
   
    pinMode(LED_PIN, OUTPUT);
}


void loop()
{
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    delay(1000);

    // Dormant because something about the setup scheduling crashes things
    // Feels like the statics are going out of scope but.... they specifically
    // are spec'd not to do that
    scheduler.process(millis());
}
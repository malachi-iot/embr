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
CONSTEXPR time_point DELAY = 500;

embr::internal::layer1::Scheduler<FunctorTraits, 5> scheduler;

void setup()
{
    static auto f_on = FunctorTraits::make_function([](time_point* wake, time_point current)
    {
        digitalWrite(LED_PIN, LOW);
        *wake += DELAY * 2;
    });

    static auto f_off = FunctorTraits::make_function([](time_point* wake, time_point current)
    {
        digitalWrite(LED_PIN, HIGH);
        *wake += DELAY * 2;
    });

    time_point start = millis() + 1000;

    scheduler.schedule(start, f_on);
    scheduler.schedule(start + DELAY, f_off);
   
    pinMode(LED_PIN, OUTPUT);
}


void loop()
{
    scheduler.process(millis());
}
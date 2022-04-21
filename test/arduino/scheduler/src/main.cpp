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
    Serial.begin(9600);

    static bool on = false;
    static auto f = FunctorTraits::make_function([](time_point* wake, time_point current)
    {
        digitalWrite(LED_PIN, on ? LOW : HIGH);
        on = !on;
        *wake += 500;
    });

    static auto f2 = FunctorTraits::make_function([](time_point* wake, time_point current)
    {
        static int counter = 0;
        *wake += 5000;
        Serial.println(++counter);
    });

    scheduler.schedule_now(f);
    scheduler.schedule_now(f2);
   
    pinMode(LED_PIN, OUTPUT);
}


void loop()
{
    scheduler.process();
}
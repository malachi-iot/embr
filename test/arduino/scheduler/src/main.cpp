#include <Arduino.h>

#include <estd/string.h>
#include <estd/chrono.h>
#include <estd/queue.h>
#include <estd/thread.h>

#include <embr/observer.h>
#include <embr/scheduler.h>

CONSTEXPR int LED_PIN = LED_BUILTIN;

void setup()
{
    pinMode(LED_PIN, OUTPUT);
}


void loop()
{
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    delay(1000);
}
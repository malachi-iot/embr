#include <Arduino.h>

#include <estd/chrono.h>
#include <estd/ostream.h>
#include <estd/thread.h>

#include <SPI.h>

#include "pins.h"

using namespace estd::chrono;

void setup()
{
    Serial.begin(115200);

    pinMode(PIN_CS, OUTPUT);

    SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
}

static const char msg[] {"Hello World!"};
static int counter = 0;

static estd::arduino_ostream clog(Serial);


void loop()
{
    estd::this_thread::sleep_for(milliseconds(250));

    char c = msg[counter];

    digitalWrite(PIN_CS, LOW);
    SPI.transfer(c);
    digitalWrite(PIN_CS, HIGH);

    clog << F("transfer(") << c << ')' << estd::endl;

    if(++counter == sizeof(msg)) counter = 0;
}
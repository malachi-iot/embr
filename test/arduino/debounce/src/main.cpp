#include <estd/chrono.h>
#include <estd/ostream.h>
#include <estd/tuple.h>

#include <embr/platform/arduino/debounce.h>

#include "conf.h"

using namespace estd::chrono_literals;

static estd::arduino_ostream cout(Serial);

using namespace embr::arduino::debounce::v1::ultimate;

void show(embr::debounce::Event e)
{
    cout << F("pin: ") << e.pin << ':' << embr::to_string(e.state) << estd::endl;
}

estd::tuple<
    Debouncer<CONFIG_GPIO_BUTTON1>
#if CONFIG_GPIO_BUTTON2
    ,Debouncer<CONFIG_GPIO_BUTTON2>
#endif
#if CONFIG_GPIO_BUTTON3
    ,Debouncer<CONFIG_GPIO_BUTTON3>
#endif
    >
    trackers;

using clock = estd::chrono::arduino_clock;
clock::time_point next_debounce, next_counter;

void setup()
{
    Serial.begin(115200);

    while(!Serial);

    cout << F("debounce test: ") << estd::tuple_size<decltype(trackers)>::value;
    cout << estd::endl;

    pinMode(CONFIG_GPIO_BUTTON1, INPUT);
#if CONFIG_GPIO_BUTTON2
    pinMode(CONFIG_GPIO_BUTTON2, INPUT);
#endif
#if CONFIG_GPIO_BUTTON3
    pinMode(CONFIG_GPIO_BUTTON3, INPUT);
#endif

    next_counter = next_debounce = clock::now();
}

void loop()
{
    static int counter = 0;
    const clock::time_point now = clock::now();

    if(now < next_debounce) return;

    next_debounce += 10ms;

    trackers.visit(Visitor{}, show);

    if(now < next_counter) return;

    next_counter += 1s;

    cout << F("counter: ") << ++counter << estd::endl;
}
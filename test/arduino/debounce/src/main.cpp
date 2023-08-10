#include <estd/ostream.h>
#include <estd/tuple.h>

#include <embr/internal/debounce/ultimate.h>

#include "conf.h"

static estd::arduino_ostream cout(Serial);

namespace debounce = embr::debounce::v1::ultimate;

template <unsigned pin>
struct Tracker : debounce::Tracker<uint16_t>
{
    using base_type = debounce::Tracker<uint16_t>;

    bool eval()
    {
        return base_type::eval(digitalRead(pin));
    }
};


struct TrackerVisitor
{
    static void eval(unsigned pin, embr::debounce::States s)
    {
        switch(s)
        {
            case embr::debounce::States::On:
                cout << F("On: ") << pin;
                break;

            case embr::debounce::States::Off:
                cout << F("Off: ") << pin;
                break;

            default: break;

            cout << estd::endl;
        }
    }

    template <unsigned I, unsigned pin>
    bool operator()(estd::variadic::instance<I, Tracker<pin>> i) const
    {
        if(i->eval())   eval(pin, i->state());
        return false;
    }
};

estd::tuple<
    Tracker<CONFIG_GPIO_BUTTON1>
#if CONFIG_GPIO_BUTTON2
    ,Tracker<CONFIG_GPIO_BUTTON2>
#endif
#if CONFIG_GPIO_BUTTON3
    ,Tracker<CONFIG_GPIO_BUTTON3>
#endif
    >
    trackers;


void setup()
{
    Serial.begin(115200);

    while(!Serial);

    cout << F("debounce test: ") << estd::tuple_size<decltype(trackers)>::value;
    cout << estd::endl;
}


void loop()
{
    trackers.visit(TrackerVisitor{});
}
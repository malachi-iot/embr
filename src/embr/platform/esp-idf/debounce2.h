#pragma once

#include "gpio.h"

#include "../../internal/debounce/ultimate.h"
#include "service/internal/event.h"

namespace embr { namespace esp_idf { namespace debounce {

inline namespace v1 { inline namespace ultimate {

//using namespace embr::debounce::v1::ultimate;

template <unsigned pin_, bool inverted>
struct Debouncer : embr::debounce::v1::ultimate::Tracker<uint16_t, inverted>
{
    using base_type = embr::debounce::v1::ultimate::Tracker<uint16_t, inverted>;

    static constexpr const unsigned pin = pin_;

    bool eval()
    {
        constexpr gpio in((gpio_num_t)pin);

        return base_type::eval(in.level());
    }
};



struct Visitor
{
    static constexpr const char* TAG = "debounce::Visitor";

    template <unsigned index, unsigned pin, bool inverted, class F>
    bool operator()(
        estd::variadic::instance<index, Debouncer<pin, inverted> > d, F&& f)
    {
        if(d->eval())
        {
            ESP_DRAM_LOGV(TAG, "notifying: %s", to_string(d->state()));
            f(embr::debounce::v1::Event{d->state(), pin});
        }

        return false;
    }
};


}}

}}}


namespace embr {

ESP_EVENT_DECLARE_BASE(DEBOUNCE_EVENT);

}


namespace embr::esp_idf::event {

inline namespace v1 { namespace internal {

template <>
struct handler<DEBOUNCE_EVENT>
{
    template <class Service>
    static constexpr bool exec(Service* s, int32_t id, void* data)
    {
        return ( s->notify(*(embr::debounce::v1::Event*) data), false );
    }
};

}}

}

#pragma once

#include "gpio.h"

#include "../../internal/debounce/ultimate.h"

namespace embr { namespace esp_idf {

namespace debounce { inline namespace v1 {


template <unsigned pin_, bool inverted>
struct Debouncer : embr::internal::DebouncerTracker<uint16_t, inverted>
{
    using base_type = embr::internal::DebouncerTracker<uint16_t, inverted>;

    static constexpr const unsigned pin = pin_;

    bool eval()
    {
        constexpr embr::esp_idf::gpio in((gpio_num_t)pin);

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

}}
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



struct debounce_visitor
{
    static constexpr const char* TAG = "debounce_visitor";

    using Item = embr::debounce::v1::Event;

    // INACTIVE, UNTESTED
    // has dummy int to temporarily keep it from conflicting
    template <unsigned index, unsigned pin, bool inverted, class F>
    bool operator()(
        estd::variadic::instance<index, Debouncer<pin, inverted> > d,
        F&& f, int)
    {
        if(d->eval())
        {
            const auto v = (embr::debounce::v1::States) d->state();
            f(Item{v, pin});
        }

        return false;
    }


    template <unsigned index, unsigned pin, bool inverted>
    bool operator()(
        estd::variadic::instance<index, Debouncer<pin, inverted> > d)
    {
        if(d->eval())
        {
            ESP_DRAM_LOGI(TAG, "on_notify: %s",
                to_string(d->state()));
        }

        return false;
    }


    // DEBT: estd::freertos::detail::queue might be better
    template <unsigned index, unsigned pin, bool inverted>
    bool operator()(
        estd::variadic::instance<index, Debouncer<pin, inverted> > d,
        estd::freertos::detail::queue<Item>& q)
    {
        if(d->eval())
        {
            const auto v = (embr::debounce::v1::States) d->state();
            ESP_DRAM_LOGV(TAG, "on_notify: %u", v);
            q.send_from_isr(Item{v, pin});
        }

        return false;
    }
};


}}

}}
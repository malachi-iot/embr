#pragma once

#include <embr/internal/debounce/ultimate.h>


enum DebounceEnum
{
    BUTTON_UNDEFINED,
    BUTTON_PRESSED,
    BUTTON_RELEASED
};


// DEBT: Can't do const here because layer1::queue's array doesn't
// play nice with it.  Upgrade layer1::queue to use 'uninitialized_array' and
// filter by is_trivial, is_trivially_copyable or is_trvially_copy_assignable
struct Item
{
    DebounceEnum state : 2;
    unsigned pin : 6;
};

const char* to_string(DebounceEnum v);

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
            const auto v = (DebounceEnum) d->state();
            ESP_DRAM_LOGV(TAG, "on_notify: %u", v);
            q.send_from_isr(Item{v, pin});
        }

        return false;
    }
};

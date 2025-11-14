#pragma once

#include <esp_log.h>

#include <string_view>

#include "../../event/v1/event.h"
#include "property.h"
#include "fwd.h"

namespace embr::esp_idf::inline prop::inline v1 {

namespace detail {

// EXPERIMENTAL
struct meta
{
    esp_event_base_t event_base;
    int32_t event_id;
    // property name
    std::string_view name;

    template <class Event, class F>
    esp_err_t handler_register(F& f)
    {
        using event_data = Event;
        return esp_event_handler_register(event_base, event_id,
            [](void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data2)
            {
                static_cast<F*>(arg)->operator()(event_id, static_cast<event_data*>(event_data2));
            }, &f);
    }
};

template <class Traits>
class state_base
{
    static constexpr const char* TAG = "state_base";

    using traits = Traits;

    static_assert(traits::is_property, "Traits don't describe a property.  Please use EMBR_IDF_PROP_TRAITS");

    using origin_type = typename traits::type::origin_type;
    using state_type = typename traits::type::state_type;

    state_type state_;

    static constexpr esp_event_base_t event_base = traits::base;
    static constexpr int32_t event_id = traits::id;

public:
    using event_data = typename traits::type;

private:
    //void post(esp_event_loop_handle_t el, esp_event_base_t event_base)
    //{
    //}

public:
    constexpr state_base() = default;
    constexpr state_base(const state_base&) = default;
    constexpr explicit state_base(const state_type& initial_state) :
        state_{initial_state} {}

    void set(state_type v, origin_type& origin,
        std::string_view name = {})
    {
        if(v == state_)     return;

        ESP_LOGV(TAG, "state_base::set: %s:%d (%s)",
            event_base, event_id, traits::id_name);
    
        event_data e(origin, state_, v, name);

        //event::post(event_base, event_id, &e);
        state_ = v;
        //event::post(event_base, event_id + 1, &e);
        event::post(event_base, event_id, &e);
    }

    void set(state_type v, origin_type& origin, esp_event_loop_handle_t event_loop,
        std::string_view name = {})
    {
        if(v == state_)     return;

        event_data e(origin, state_, v, name);

        //event::post(event_loop, event_base, event_id, &e);
        state_ = v;
        //event::post(event_loop, event_base, event_id + 1, &e);
        event::post(event_loop, event_base, event_id, &e);
    }

    constexpr state_type get() const
    {
        return state_;
    }

    // EXPERIMENTAL
    state_base& operator =(state_type v)
    {
        static_assert(traits::o != nullptr, "Origin pointer must be provided in traits");
    
        state_ = v;
        return *this;
    }

    constexpr operator state_type() const
    {
        return state_;
    }

    // Does not support the wildcard event idea
    template <class F>
    static esp_err_t handler_register(F& f)
    {
        return esp_event_handler_register(event_base, event_id,
            [](void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data2)
            {
                static_cast<F*>(arg)->operator()(static_cast<event_data*>(event_data2));
            }, &f);
    }

    template <class F>
    static esp_err_t handler_register(esp_event_loop_handle_t loop_handle, F& f)
    {
        return esp_event_handler_register_with(loop_handle, event_base, event_id,
            [](void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data2)
            {
                static_cast<F*>(arg)->operator()(static_cast<event_data*>(event_data2));
            }, &f);
    }
};

}

}

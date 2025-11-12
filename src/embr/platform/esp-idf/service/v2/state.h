#pragma once

#include <string_view>

#include "../../event.h"

template <class Tag, int32_t _id>
struct embr_idf_property_traits
{
    static constexpr bool specialized = false;
    static constexpr const char* id_name = "unspecified";
    using type = void;
};

#define EMBR_IDF_PROP_TRAITS(_enum, _id, _type) \
template <> \
struct embr_idf_property_traits<_enum, _id> \
{ \
    static constexpr bool specialized = true; \
    static constexpr int32_t id = _id; \
    static constexpr const char* id_name = #_id; \
    static constexpr const char* tag = #_enum; \
    using type = _type; \
};

namespace embr::esp_idf::service::inline v2 {

template <class Tag, int32_t id>
using property_traits = embr_idf_property_traits<Tag, id>;

namespace detail {

template <class State, class Origin, Origin* o = {}>
struct state_base_traits
{
    using state_type = State;
    using origin_type = Origin;

    //static constexpr esp_event_base_t event_base = eb;
    static constexpr origin_type* origin = o;

    struct event_data
    {
        origin_type& origin;
        state_type changing_state;
        state_type changed_state;
        // property name
        std::string_view name;
    };

    static constexpr event_data make_event_data(
        origin_type& origin, state_type changing_state,
        state_type changed_state, std::string_view name = {})
    {
        return{origin, changing_state, changed_state, name};
    }
};

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
    using traits = Traits;
    using origin_type = typename traits::origin_type;
    using state_type = typename traits::state_type;

    state_type state_;

public:
    using event_data = typename traits::event_data;

private:
    //void post(esp_event_loop_handle_t el, esp_event_base_t event_base)
    //{
    //}

public:
    constexpr state_base() = default;
    constexpr state_base(const state_base&) = default;
    constexpr explicit state_base(state_type initial_state) :
        state_{initial_state} {}

    void set(state_type v, origin_type& origin,
        esp_event_base_t event_base, int32_t event_id, std::string_view name = {})
    {
        if(v == state_)     return;

        event_data e = traits::make_event_data(origin, state_, v, name);

        event::post(event_base, event_id, &e);
        state_ = v;
        event::post(event_base, event_id + 1, &e);
    }

    void set(state_type v, origin_type& origin, esp_event_loop_handle_t event_loop,
        esp_event_base_t event_base, int32_t event_id, std::string_view name = {})
    {
        if(v == state_)     return;

        event_data e = traits::make_event_data(origin, state_, v, name);

        event::post(event_loop, event_base, event_id, &e);
        state_ = v;
        event::post(event_loop, event_base, event_id + 1, &e);
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
    static esp_err_t handler_register(esp_event_base_t event_base, int32_t event_id, F& f)
    {
        return esp_event_handler_register(event_base, event_id,
            [](void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data2)
            {
                static_cast<F*>(arg)->operator()(static_cast<event_data*>(event_data2));
            }, &f);
    }

    template <class F>
    static esp_err_t handler_register(esp_event_loop_handle_t loop_handle,
        esp_event_base_t event_base, int32_t event_id, F& f)
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

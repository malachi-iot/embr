#pragma once

#include <string_view>

#include "../../event.h"

namespace embr::esp_idf::service::inline v2 {

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
};

}

}

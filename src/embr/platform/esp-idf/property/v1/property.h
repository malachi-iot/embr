#pragma once

#include <string_view>

#include "../../event/v1/traits.h"
#include "../../event/v1/event.h"
#include "concepts.h"
#include "macro.h"
#include "fwd.h"

namespace embr::esp_idf::inline prop::inline v1 {

template <class T>
struct property_event_data_base
{
#if __GXX_RTTI
    // In the event RTTI is on, we'd really like to
    // double-check this guy on handler firing
    virtual ~property_event_data_base() = default;
#endif

    using value_type = T;

    value_type changing_state;
    value_type changed_state;

    constexpr property_event_data_base(
        const T& changing_state_,
        const T& changed_state_) :
        changing_state(changing_state_),
        changed_state(changed_state_)
    {}
};

// DEBT: Consider moving this to a non-idf specific area
template <class T, class Origin>
struct property_event_data : property_event_data_base<T>
{
    using base_type = property_event_data_base<T>;
    using origin_type = Origin;

    Origin* const origin;
    // property name
    std::string_view name;

    constexpr property_event_data(
        Origin* origin_,
        const T& changing_state_,
        const T& changed_state_,
        std::string_view name_) :
        base_type(changing_state_, changed_state_),
        origin(origin_),
        name(name_)
    {}
};

namespace detail {

// DEBT: Put this elsewhere, or above macros elsewhere
template <class Traits, bool send_to_default_loop>
class property
{
    using traits = Traits;
public:
    using event_type = typename traits::data_type;
    using value_type = typename event_type::value_type;
    using origin_type = typename event_type::origin_type;

private:
    static constexpr traits::type id = traits::id;

    value_type value_;

public:
    template <class ...Args>
    constexpr explicit property(Args&&... args) :
        value_(std::forward<Args>(args)...) {}

    constexpr operator const value_type&() const
    {
        return value_;
    }

    constexpr const value_type& get() const
    {
        return value_;
    }

    // DEBT: Do concept here
    template <class ...LoopHandles>
    bool set(const value_type& v, origin_type* origin, LoopHandles... loop_handles)
    {
        if(v == value_)     return false;

        const event_type e{origin, value_, v, {}};

        value_ = v;

        if constexpr(send_to_default_loop || sizeof...(loop_handles) == 0)
        {
            event::post<id>(&e);
        }

        return (event::post<id>(loop_handles, &e) + ... + 0);
    }

    // DEBT: Stop-gap, I don't think I want these here
    static esp_err_t handler_register(auto& f)
    {
        return event::handler_register<id>(f);
    }

    static esp_err_t handler_register(esp_event_loop_handle_t loop_handle, auto& f)
    {
        return event::handler_register<id>(loop_handle, f);
    }
};

}

}
#pragma once

#include <string_view>

#include <esp_check.h>

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

template <class Traits, bool send_to_default_loop>
class property
{
    using traits = Traits;
    static constexpr const char* TAG = traits::id_name;

public:
    using event_type = typename traits::data_type;
    using value_type = typename event_type::value_type;
    using origin_type = typename event_type::origin_type;

private:
    static constexpr traits::type id = traits::id;

    value_type value_;

    esp_err_t post(esp_event_loop_handle_t loop_handle, const event_type* e, esp_err_t* ret)
    {
        esp_err_t r = event::post<id>(loop_handle, e);

        if(r != ESP_OK)
        {
            ESP_LOGE(TAG, "Could not post to loop: %p", loop_handle);
            // Update ret only on error, so that subsequent successes don't make us forget
            *ret = r;
        }

        return r;
    }

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
    esp_err_t set(const value_type& v, origin_type* origin, LoopHandles... loop_handles)
    {
        if(v == value_)     return ESP_OK;

        const event_type e{origin, value_, v, {}};

        value_ = v;

        esp_err_t ret = ESP_OK;

        if constexpr(send_to_default_loop || sizeof...(loop_handles) == 0)
        {
            ESP_RETURN_ON_ERROR(event::post<id>(&e), TAG,
                "post to default loop failed");
        }

        //[[maybe_unused]] int dummy =
        //(event::post<id>(loop_handles, &e) + ... + 0);
        (void)(post(loop_handles, &e, &ret), ...);

        return ret;
    }
};

}

}
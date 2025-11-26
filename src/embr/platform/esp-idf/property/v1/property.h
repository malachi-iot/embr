#pragma once

#include <string_view>

#include "../../event/v1/traits.h"
#include "concepts.h"
#include "macro.h"
#include "fwd.h"

// DEBT: Really I ought to change these to EMBR_ESP_PROP_TRAITS to match the event traits,
// but I like how these look as IDF.

#define EMBR_IDF_PROP_TRAITS(event_base, event_id, _type, origin) \
template <> \
struct embr_esp_event_traits_legacy<event_base, event_id> \
{ \
    EMBR_ESP_EVENT_TRAITS_BODY(event_base, event_id) \
    static constexpr bool is_property = true; \
    using type = ::embr::esp_idf::prop::property_event_data<_type, origin>; \
    static constexpr const char* type_name = "property<_type, origin>"; \
    /* experimental */ static constexpr int32_t changing_id = id + 10000; \
    /* experimental */ static constexpr const char* property_name = #origin ".TBD"; \
};


#define EMBR_IDF_PROP_TRAITS_NS(ns, event_base, event_id, type, origin) \
EMBR_IDF_PROP_TRAITS(ns::event_base, ns::event_id, type, ns::origin)

#define EMBR_IDF_PROP_DECLARE(event_id, name) \
    using name ## _ ## type = ::embr::esp_idf::prop::v1::property_legacy<event_base, event_id>;  \
    using name ## _event_data = typename name ## _ ## type::event_data; \
    /* experimental */ static constexpr const char* prop ## _ ## name ## _id = #name;

#define EMBR_IDF_PROP_PROVIDER(_event_base, origin)    \
    static constexpr esp_event_base_t event_base = _event_base; \
    static constexpr const char* origin_name = #origin;


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

template <auto event_id>
using event_data = const event::event_traits<event_id>::data_type;

namespace detail {

// DEBT: Put this elsewhere, or above macros elsewhere
template <class Traits, bool send_to_default_loop>
class property
{
    using traits = Traits;
    using event_type = typename traits::data_type;
    using value_type = typename event_type::value_type;
    using origin_type = typename event_type::origin_type;

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
};

}

}
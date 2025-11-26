#pragma once

#include <string_view>

#include "../../event/v1/traits.h"
#include "concepts.h"
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


template <class Enum>
struct embr_esp_prop_provider_traits
{
    static constexpr bool is_specialized = false;
    using type = void;
};



#define EMBR_ESP_EVENT_DECLARE_PROP_BASE_NS(ns, id, provider) \
namespace ns { \
using id ## _preserved = id; \
ESP_EVENT_DECLARE_BASE(id); \
} \
template <> struct embr_esp_prop_provider_traits<ns::id ## _preserved> \
{ \
    static constexpr bool is_specialized = true; \
    using type = ns::provider; \
}; \
EMBR_ESP_EVENT_BASE_TRAITS(ns, id);

#define EMBR_ESP_PROP_TRAITS(event_id, data) \
template <> \
struct embr_esp_event_traits_exp<event_id> : ::embr::internal::event_traits_parent<event_id> \
{ \
    using provider_traits = embr_esp_prop_provider_traits<decltype(event_id)>; \
    static constexpr bool is_specialized = true; \
    static constexpr bool is_property = true; \
    static constexpr const char* id_name = #event_id; \
    using data_type = ::embr::esp_idf::prop::property_event_data<data, provider_traits::type>; \
    static constexpr const char* data_name = "property_event_data(" #data ")"; \
    using payload_type = data; \
    static constexpr const char* payload_name = #data; \
};


// EXPERIMENTAL, and not working (see service.h notes)
#define EMBR_IDF_PROP_TRAITS2(ns, origin, event_id, type) \
EMBR_IDF_PROP_TRAITS(ns::origin::event_base, \
    ns::event_id,    \
    decltype(std::declval<ns::origin>().accessor()), \
    type)

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

// DEBT: Consider moving this to a non-idf specific area
template <class T, class Origin>
struct property_event_data
{
#if __GXX_RTTI
    // In the event RTTI is on, we'd really like to
    // double-check this guy on handler firing
    virtual ~property_event_data() = default;
#endif

    using value_type = T;
    using origin_type = Origin;

    Origin* const origin;
    T changing_state;
    T changed_state;
    // property name
    std::string_view name;
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
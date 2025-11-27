#pragma once

#include "../../event/v1/traits.h"

#define EMBR_ESP_EVENT_DECLARE_PROP_BASE_NS(ns, id, provider) \
namespace ns { \
using id ## _preserved = id; \
ESP_EVENT_DECLARE_BASE(id); \
} \
template <> struct embr_esp_prop_provider_traits<ns::id ## _preserved> : \
    embr::internal::specialized_type<ns::provider> {};  \
EMBR_ESP_EVENT_BASE_TRAITS(ns, id);

// Not ready yet
#define EMBR_ESP_EVENT_DECLARE_PROP_BASE_NO_NS(id, provider) \
using id ## _preserved = id; \
ESP_EVENT_DECLARE_BASE(id); \
template <> struct embr_esp_prop_provider_traits<id ## _preserved> \
{ \
    static constexpr bool is_specialized = true; \
    using type = provider; \
}; \
EMBR_ESP_EVENT_BASE_TRAITS(id);


#define EMBR_ESP_PROP_TRAITS(event_id, data) \
template <> \
struct embr_esp_event_traits<event_id> : ::embr::internal::event_traits_parent<event_id> \
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


// Not ready yet
#define EMBR_ESP_EVENT_DECLARE_PROP_BASE(...)  \
_GET_MACRO(__VA_ARGS__, \
    EMBR_ESP_EVENT_DECLARE_PROP_BASE_NS, \
    EMBR_ESP_EVENT_DECLARE_PROP_BASE_NO_NS)(__VA_ARGS__)
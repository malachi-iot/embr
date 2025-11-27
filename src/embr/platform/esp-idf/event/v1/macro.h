#pragma once

#define EMBR_ESP_EVENT_TRAITS(event_id, data) \
template <> \
struct embr_esp_event_traits<event_id> : ::embr::internal::event_traits_parent<event_id> \
{ \
    static constexpr bool is_specialized = true; \
    static constexpr const char* id_name = #event_id; \
    using data_type = data; \
    static constexpr const char* data_name = #data; \
};


#define EMBR_ESP_EVENT_BASE_TRAITS(ns, event_base) \
template <> \
struct embr_esp_event_base_traits<ns::event_base ## _preserved> \
{ \
    static constexpr bool is_specialized = true; \
    using type = ns::event_base ## _preserved; \
    static esp_event_base_t name() { return ns::event_base; } \
};


#define EMBR_ESP_EVENT_DECLARE_BASE_NS(ns, id) \
namespace ns { \
using id ## _preserved = id; \
ESP_EVENT_DECLARE_BASE(id); \
} \
EMBR_ESP_EVENT_BASE_TRAITS(ns, id);


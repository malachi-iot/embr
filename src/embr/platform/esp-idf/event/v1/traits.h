#pragma once

#include <esp_event.h>

#include "fwd.h"

// At global namespace to simplify specialization

template <esp_event_base_t event_base, int32_t event_id>
struct embr_esp_event_traits
{
    static constexpr bool is_specialized = false;
    static constexpr bool is_property = false;
    static constexpr int32_t id = event_id;
    static constexpr const char* id_name = "unspecified";
    static constexpr const char* base = event_base;
    using type = void;
    static constexpr const char* type_name = id_name;
};

#define EMBR_ESP_EVENT_TRAITS_BODY(event_base, event_id) \
    static constexpr bool is_specialized = true; \
    static constexpr int32_t id = event_id; \
    static constexpr const char* id_name = #event_id; \
    static constexpr const char* base = event_base;


#define EMBR_ESP_EVENT_TRAITS(event_base, event_id, payload) \
template <> \
struct embr_esp_event_traits<event_base, event_id> \
{ \
    EMBR_ESP_EVENT_TRAITS_BODY(event_base, event_id) \
    static constexpr bool is_property = false; \
    using type = payload; \
    static constexpr const char* type_name = #payload; \
};

#define EMBR_ESP_EVENT_DECLARE_BASE(id) constexpr const char id[] = #id

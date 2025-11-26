#pragma once

#include <esp_event.h>

#include "fwd.h"

// At global namespace to simplify specialization

// FIX: specializing on esp_event_base_t is troubled.  constexpr char* doesn't
// hop across TUs well it seems.  Try <auto event_id> and declspec out enum_type (if needed)
// Keep event_base as part of the macro.
// Also, try testing hop-TU in unit test if we can (synthetic lib maybe? UNIT_TEST flag
// to put into main lib?)

namespace embr::internal {

template <class T = void, bool specialized = true>
struct specialized_type
{
    static constexpr bool is_specialized = specialized;
    using type = T;
};

// Normally I call a base class 'base' but in this case we'll call it parent
// to disambiguate from event_base
template <auto event_id>
struct event_traits_parent
{
    using type = decltype(event_id);
    using base_traits = embr_esp_event_base_traits<type>;
    static const char* base() { return base_traits::name(); }
    static constexpr bool is_property = false;
    static constexpr type id = event_id;
};

}

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

template <class EventEnum>
struct embr_esp_event_base_traits :
    embr::internal::specialized_type<EventEnum, false>
{
};


template <auto event_id>
struct embr_esp_event_traits_exp :
    embr::internal::specialized_type<decltype(event_id)>
{
    static constexpr bool is_property = false;
    static constexpr decltype(event_id) id = event_id;
    using payload_type = void;
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

#define EMBR_ESP_EVENT_TRAITS_EXP(event_base, event_id, payload) \
template <> \
struct embr_esp_event_traits_exp<event_id> \
{ \
    using type = decltype(event_id);   \
    static constexpr bool is_specialized = true; \
    static constexpr type id = event_id; \
    static constexpr const char* id_name = #event_id; \
    static constexpr bool is_property = false; \
    using payload_type = payload; \
    static constexpr const char* type_name = #payload; \
    static const char* base() { return event_base; } \
};

#define EMBR_ESP_EVENT_TRAITS_EXP2(event_id, data) \
template <> \
struct embr_esp_event_traits_exp<event_id> : ::embr::internal::event_traits_parent<event_id> \
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

// FIX: Trouble in TU paradise
#define EMBR_ESP_EVENT_DECLARE_BASE(id) constexpr const char id[] = #id

#define _GET_MACRO(_1, _2, NAME, ...) NAME

// FIX: Works OK except not inside a namespace due to specialization
#define EMBR_ESP_EVENT_DECLARE_BASE_EXP(id) \
using id ## _preserved = id; \
ESP_EVENT_DECLARE_BASE(id); \
EMBR_ESP_EVENT_BASE_TRAITS(, id);

#define EMBR_ESP_EVENT_DECLARE_BASE_NS(ns, id) \
namespace ns { \
using id ## _preserved = id; \
ESP_EVENT_DECLARE_BASE(id); \
} \
EMBR_ESP_EVENT_BASE_TRAITS(ns, id);

#define EMBR_ESP_EVENT_DECLARE_BASE_EXP2(...)  _GET_MACRO(__VA_ARGS__, EMBR_ESP_EVENT_DECLARE_BASE_NS, EMBR_ESP_EVENT_DECLARE_BASE_EXP)(__VA_ARGS__)
#pragma once

#include "../../event.h"

// DEBT: Do consolidating EMBR_IDF_EVENT_TRAITS_BASE
#define EMBR_IDF_PROP_TRAITS(event_base, event_id, _type, origin) \
template <> \
struct embr_idf_event_traits<event_base, event_id> \
{ \
    static constexpr bool specialized = true; \
    static constexpr bool is_property = true; \
    static constexpr int32_t id = event_id; \
    static constexpr const char* id_name = #event_id; \
    static constexpr const char* base = event_base; \
    using type = ::embr::esp_idf::service::property_event_data<_type, origin>; \
    static constexpr const char* type_name = "TBD"; \
    /* experimental */ static constexpr int32_t changing_id = id + 10000; \
};

// DEBT: Use a different namespace, and so far the content of this namespace is not idf specific
namespace embr::esp_idf::service::inline v2 {

template <class State, class Origin>
struct property_event_data
{
    using state_type = State;
    using origin_type = Origin;

    Origin& origin;
    State changing_state;
    State changed_state;
    // property name
    std::string_view name;
};


}

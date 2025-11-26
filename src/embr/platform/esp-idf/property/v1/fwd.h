#pragma once

// DEBT: A bit of fragility to including this guy (I don't think he's as reliable
// as esp_event.h)
//#include <esp_event_base.h>
#include "../../event.h"

// DEBT: We have an inline v2 about
namespace embr::esp_idf::inline prop::inline v1 {

namespace detail {

template <class Traits>
class state_base;

template <class Traits, bool send_to_default_loop>
class property;

}

template <esp_event_base_t event_base, int32_t _id>
using property_legacy = detail::state_base<embr_esp_event_traits_legacy<event_base, _id>>;

template <auto event_id, bool send_to_default_loop = true, class Traits = event::event_traits<event_id>>
using property = detail::property<Traits, send_to_default_loop>;
    
}

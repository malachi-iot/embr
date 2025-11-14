#pragma once

// DEBT: A bit of fragility to including this guy (I don't think he's as reliable
// as esp_event.h)
//#include <esp_event_base.h>
#include "../../event.h"

namespace embr::esp_idf::service::inline v2 {

namespace detail {

template <class Traits>
class state_base;

}

}

namespace embr::esp_idf::inline prop::inline v1 {

template <esp_event_base_t event_base, int32_t _id>
using property = service::v2::detail::state_base<embr_idf_event_traits<event_base, _id>>;
    
}

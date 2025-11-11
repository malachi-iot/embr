#pragma once

#include "../../../../service/v2/enum.h"

#include "state.h"

namespace embr::esp_idf::service::inline v2 {

enum SERVICE_EVENTS
{
    SERVICE_CHANGING_STATE,
    SERVICE_CHANGED_STATE,

    DRIVER_CHANGING_STATE,
    DRIVER_CHANGED_STATE,

    SUBSYSTEM_CHANGING_STATE,
    SUBSYSTEM_CHANGED_STATE,

    APP_CHANGING_STATE,
    APP_CHANGED_STATE,

    USER1_CHANGING_STATE,
    USER1_CHANGED_STATE,
};

ESP_EVENT_DECLARE_BASE(SERVICE_EVENTS);

class service : public embr::service::v2::detail::service
{
    using base_type = embr::service::v2::detail::service;
    using typename base_type::states;
    using typename base_type::substates;

    detail::state_base<detail::state_base_traits<
        substates, service>> state_;

protected:
    void state(substates s, int32_t event_id = SERVICE_CHANGING_STATE)
    {
        state_.set(s, *this, SERVICE_EVENTS, event_id);
    }

public:

};

}

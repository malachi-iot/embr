#pragma once

#include "../../../../service/v2/enum.h"

#include "../../property/v1/property.h"

namespace embr::esp_idf::service::inline v2 {

enum SERVICE_EVENTS
{
    SERVICE_CHANGING_STATE,
    SERVICE_CHANGED_STATE,
};

class service;

}

EMBR_ESP_EVENT_DECLARE_PROP_BASE_NS(embr::esp_idf::service::v2, SERVICE_EVENTS, service);
EMBR_ESP_PROP_TRAITS(
    embr::esp_idf::service::v2::SERVICE_CHANGED_STATE,
    embr::service::v2::detail::service::substates);

namespace embr::esp_idf::service::inline v2 {

class service : public embr::service::v2::detail::service
{
    using base_type = embr::service::v2::detail::service;

public:
    // DEBT: Defunct
    struct property
    {
        static constexpr const char* state = "service.state";
    };

    using state_type = prop::v1::property<SERVICE_CHANGED_STATE>;
    using state_event_data = typename state_type::event_type;

protected:
    using typename base_type::states;
    using typename base_type::substates;

    state_type state_{Unstarted};

    void state(substates s)
    {
        state_.set(s, this);
    }

    void state(substates s, esp_event_loop_handle_t loop_handle)
    {
        state_.set(s, this, loop_handle);
    }

public:

    constexpr substates substate() const { return state_.get(); }
    constexpr states state() const
    {
        return static_cast<states>(state_.get() >> separator);
    }

    // NOTE: Don't want this, it doesn't help with 'this' comparison
    // and otherwise maps to handler_register directly
    //static esp_err_t on_state_changed(F& f)
};

}

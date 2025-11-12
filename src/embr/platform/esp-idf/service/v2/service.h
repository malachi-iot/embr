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

enum PROPERTY_EVENTS
{
    SERVICE_PROPERTY_CHANGED,
    DRIVER_PROPERTY_CHANGED,
    SUBSYSTEM_PROPERTY_CHANGED,
    APP_PROPERTY_CHANGED,
    USER1_PROPERTY_CHANGED,
};

}

/*
EMBR_IDF_PROP_TRAITS(
    embr::esp_idf::service::SERVICE_EVENTS,
    embr::esp_idf::service::SERVICE_CHANGED_STATE,
    embr::service::v2::service::substates);*/

namespace embr::esp_idf::service::inline v2 {

EMBR_ESP_EVENT_DECLARE_BASE(SERVICE_EVENTS);
ESP_EVENT_DECLARE_BASE(PROPERTY_EVENTS);

class service : public embr::service::v2::detail::service
{
    using base_type = embr::service::v2::detail::service;

public:
    struct property
    {
        static constexpr const char* state = "service.state";
    };

protected:
    using typename base_type::states;
    using typename base_type::substates;
    using state_prop_traits = event_traits<SERVICE_EVENTS, SERVICE_CHANGED_STATE>;
    using state_traits = detail::state_base_traits<substates, service>;
    using state_base = detail::state_base<state_traits>;

    state_base state_{Unstarted};

    void state(substates s, int32_t event_id = SERVICE_CHANGING_STATE)
    {
        state_.set(s, *this, SERVICE_EVENTS, event_id, property::state);
    }

    void state(substates s, esp_event_loop_handle_t loop_handle, int32_t event_id = SERVICE_CHANGING_STATE)
    {
        state_.set(s, *this, loop_handle, SERVICE_EVENTS, event_id, property::state);
    }

public:
    using state_event_data = typename state_traits::event_data;

    constexpr substates substate() const { return state_.get(); }
    constexpr states state() const
    {
        return static_cast<states>(state_.get() >> separator);
    }

    template <class F>
    static esp_err_t handler_register(int32_t event_id, F& f)
    {
        return state_base::handler_register(SERVICE_EVENTS, event_id, f);
    }

    template <class F>
    static esp_err_t handler_register(esp_event_loop_handle_t loop_handle, int32_t event_id, F& f)
    {
        return state_base::handler_register(loop_handle, SERVICE_EVENTS, event_id, f);
    }

    template <class F>
    static esp_err_t on_state_changed(F& f)
    {
        return state_base::handler_register(SERVICE_EVENTS, SERVICE_CHANGED_STATE, f);
    }
};

}

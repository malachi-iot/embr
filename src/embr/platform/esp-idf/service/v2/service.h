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

public:
    struct property
    {
        static constexpr const char* state = "service.state";
    };

protected:
    using typename base_type::states;
    using typename base_type::substates;
    using traits = detail::state_base_traits<substates, service>;

    detail::state_base<traits> state_{Unstarted};

    void state(substates s, int32_t event_id = SERVICE_CHANGING_STATE)
    {
        state_.set(s, *this, SERVICE_EVENTS, event_id, property::state);
    }

    void state(substates s, esp_event_loop_handle_t loop_handle, int32_t event_id = SERVICE_CHANGING_STATE)
    {
        state_.set(s, *this, loop_handle, SERVICE_EVENTS, event_id, property::state);
    }

public:
    using event_data = typename traits::event_data;

    constexpr substates substate() const { return state_.get(); }
    constexpr states state() const
    {
        return static_cast<states>(state_.get() >> separator);
    }

    template <class F>
    static esp_err_t handler_register(int32_t event_id, F& f)
    {
        return esp_event_handler_register(SERVICE_EVENTS, event_id,
            [](void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data2)
            {
                static_cast<F*>(arg)->operator()(event_id, static_cast<event_data*>(event_data2));
            }, &f);
    }

    template <class F>
    static esp_err_t handler_register(esp_event_loop_handle_t loop_handle, int32_t event_id, F& f)
    {
        return esp_event_handler_register_with(loop_handle, SERVICE_EVENTS, event_id,
            [](void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data2)
            {
                static_cast<F*>(arg)->operator()(event_id, static_cast<event_data*>(event_data2));
            }, &f);
    }
};

}

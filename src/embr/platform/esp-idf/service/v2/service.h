#pragma once

#include "../../../../service/v2/enum.h"

#include "../../property/v1/state.h"

namespace embr::esp_idf::service::inline v2 {

enum SERVICE_EVENTS
{
    SERVICE_CHANGING_STATE,
    SERVICE_CHANGED_STATE,
};

// DEBT: As we transition to EMBR_ESP_EVENT_DECLARE_BASE
using SERVICE_EVENTS_preserved = SERVICE_EVENTS;

// FIX: Has serious problems
EMBR_ESP_EVENT_DECLARE_BASE_LEGACY(SERVICE_EVENTS);
class service;

}

EMBR_ESP_EVENT_BASE_TRAITS(embr::esp_idf::service::v2, SERVICE_EVENTS);

/*
EMBR_IDF_PROP_TRAITS(
    embr::esp_idf::service::SERVICE_EVENTS,
    embr::esp_idf::service::SERVICE_CHANGED_STATE,
    embr::service::v2::service::substates,
    embr::esp_idf::service::v2::service);   */

// Nice try, but no banana:
// 1. Placing above 'service' means we can't deduce event_base
// 2. Placing below 'service' means state_base won't enjoy our specialization
//EMBR_IDF_PROP_TRAITS2(embr::esp_idf::service::v2, service, SERVICE_CHANGED_STATE,
//    embr::service::v2::service::substates);

EMBR_IDF_PROP_TRAITS_NS(embr::esp_idf::service::v2,
    SERVICE_EVENTS, SERVICE_CHANGED_STATE,
    embr::service::v2::service::substates,
    service);

namespace embr::esp_idf::service::inline v2 {

class service : public embr::service::v2::detail::service
{
    using base_type = embr::service::v2::detail::service;

public:
    struct property
    {
        static constexpr const char* state = "service.state";
    };

    EMBR_IDF_PROP_PROVIDER(SERVICE_EVENTS, service);
    EMBR_IDF_PROP_DECLARE(SERVICE_CHANGED_STATE, state);

protected:
    using typename base_type::states;
    using typename base_type::substates;

    state_type state_{Unstarted};

    void state(substates s)
    {
        state_.set(s, this, property::state);
    }

    void state(substates s, esp_event_loop_handle_t loop_handle)
    {
        state_.set(s, this, loop_handle, property::state);
    }

public:

    constexpr substates substate() const { return state_.get(); }
    constexpr states state() const
    {
        return static_cast<states>(state_.get() >> separator);
    }

    template <class F>
    static esp_err_t on_state_changed(F& f)
    {
        return state_type::handler_register(f);
    }

    template <class F>
    static esp_err_t on_state_changed(esp_event_loop_handle_t loop_handle, F& f)
    {
        return state_type::handler_register(loop_handle, f);
    }
};

}

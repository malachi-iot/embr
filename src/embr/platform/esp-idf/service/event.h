#pragma once

#include <esp_wifi.h>

#include <embr/service.h>

#include "internal/ip.h"
#include "result.h"
#include "internal/wifi.h"

namespace embr::esp_idf {

namespace service { inline namespace v1 {

// Specifically describes 'default' event loop
struct EventLoop : embr::service::v1::SparseService
{
    typedef EventLoop this_type;

    static constexpr const char* TAG = "EventLoop";
    static constexpr const char* name() { return TAG; }

    struct event
    {
        template <typename TEventId>
        using e = embr::esp_idf::event::runtime<TEventId>;
    };

    template <class Runtime, const esp_event_base_t&>
    static void event_handler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data);

    template <const esp_event_base_t&, class Runtime>
    static esp_err_t handler_register(int32_t, Runtime* runtime);

    EMBR_SERVICE_RUNTIME_BEGIN(embr::service::v1::SparseService)
    
        state_result on_start();
    
        template <const esp_event_base_t& event_base>
        friend struct esp_idf::event::v1::internal::handler;

        template <const esp_event_base_t&>
        esp_err_t handler_register(int32_t = ESP_EVENT_ANY_ID);

        template <const esp_event_base_t&>
        esp_err_t handler_register(int32_t, esp_event_handler_instance_t*);

    EMBR_SERVICE_RUNTIME_END
};


struct UserEvent : embr::service::v1::Service
{
    typedef UserEvent this_type;

    static constexpr const char* TAG = "UserEvent";
    static constexpr const char* name() { return TAG; }

    esp_event_loop_handle_t* handle_;

    EMBR_SERVICE_RUNTIME_BEGIN(embr::service::v1::Service)

        state_result on_start(const esp_event_loop_args_t*);

        template <const esp_event_base_t& event_base>
        friend struct esp_idf::event::v1::internal::handler;

        template <const esp_event_base_t&>
        esp_err_t handler_register(int32_t = ESP_EVENT_ANY_ID);

        template <const esp_event_base_t&>
        esp_err_t handler_register(int32_t, esp_event_handler_instance_t*);

    EMBR_SERVICE_RUNTIME_END
};


}}

}

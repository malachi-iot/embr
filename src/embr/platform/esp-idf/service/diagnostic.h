#pragma once

#include "core.h"

namespace embr { namespace esp_idf {

namespace service { inline namespace v1 {

struct Diagnostic
{
    static constexpr const char* TAG = "Diagnostic";

    template <class TRuntime>
    static void on_notify(embr::event::PropertyChanged<embr::Service::id::state> e,
        const TRuntime& runtime)
    {
        // DEBT: Usage of this runtime portion a little too magic
        const auto& r = runtime.impl();
        unsigned level = e.value == Service::Error ?
            ESP_LOG_WARN :
            ESP_LOG_INFO;

        ESP_LOG_LEVEL(level, TAG, "service [%s:%s] state: %s:%s",
            r.name(),
            r.instance(),
            to_string(e.value),
            to_string(r.substate()));
    }

    template <class TRuntime>
    static void on_notify(embr::event::PropertyChanged<embr::Service::id::substate> e,
        const TRuntime& runtime)
    {
        // DEBT: Usage of this runtime portion a little too magic
        const auto& r = runtime.impl();

        ESP_LOGD(TAG, "service [%s:%s] sub state: %s",
            r.name(),
            r.instance(),
            to_string(e.value));
    }
};

}}

}}

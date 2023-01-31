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
        ESP_LOGI(TAG, "service [%s] state: %s:%s", r.name(),
            to_string(e.value),
            to_string(r.substate()));
    }

    template <class TRuntime>
    static void on_notify(embr::event::PropertyChanged<embr::Service::id::substate> e,
        const TRuntime& runtime)
    {
        // DEBT: Usage of this runtime portion a little too magic
        const auto& r = runtime.impl();
        ESP_LOGD(TAG, "service [%s] sub state: %s", r.name(), to_string(e.value));
    }
};

}}

}}

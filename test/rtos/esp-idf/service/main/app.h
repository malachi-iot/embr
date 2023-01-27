#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>

#include "timer.hpp"

struct App
{
    static constexpr const char* TAG = "App";

    void on_notify(embr::event::PropertyChanged<
        embr::Service::id::config<const gptimer_config_t*> > e)
    {
        ESP_LOGI(TAG, "timer configured");
    }

    void on_notify(embr::event::PropertyChanged<TimerService::id::timer> e)
    {
        ESP_DRAM_LOGI(TAG, "value = %" PRIu32, e.value);
    }

    template <class TRuntime>
    void on_notify(embr::event::PropertyChanged<embr::Service::id::state> e,
        TRuntime& runtime)
    {
        // DEBT: Usage of this runtime portion a little too magic
        const auto& r = runtime.impl();
        ESP_LOGI(TAG, "service [%s] state: %u", r.name(), e.value);
    }
};
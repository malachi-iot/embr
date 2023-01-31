#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>

#include "timer.hpp"

struct App
{
    static constexpr const char* TAG = "App";

    void on_notify(embr::event::PropertyChanged<
        embr::Service::id::config<const gptimer_config_t&> > e)
    {
        ESP_LOGI(TAG, "timer configured: resolution_hz=%" PRIu32,
            e.value.resolution_hz);
    }

    void on_notify(embr::event::PropertyChanged<TimerService::id::timer> e)
    {
        ESP_DRAM_LOGI(TAG, "value = %" PRIu32, e.value);
    }
};
#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>

#include "timer.h"
#include "system.h"

struct App
{
    static constexpr const char* TAG = "App";

    template <class T>
    using changed = embr::event::PropertyChanged<T>;

    void on_notify(changed<
        embr::Service::id::config<const gptimer_config_t&> > e)
    {
        ESP_LOGI(TAG, "timer configured: resolution_hz=%" PRIu32,
            e.value.resolution_hz);
    }

    void on_notify(changed<TimerService::id::timer> e)
    {
        ESP_DRAM_LOGI(TAG, "value = %" PRIu32, e.value);
    }

    void on_notify(changed<embr::Service::id::substate> e,
        services::PowerManager&)
    {
        if(e.value == embr::Service::Waking)
            ESP_LOGI(TAG, "bringup");
        else if(e.value == embr::Service::Sleeping)
            ESP_LOGI(TAG, "sleeping");
    }
};
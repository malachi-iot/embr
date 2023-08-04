#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/gptimer.h>
#include <embr/platform/esp-idf/service/pm.h>

#include "filter.h"

struct App
{
    static constexpr const char* TAG = "App";

    template <class T>
    using changed = embr::event::PropertyChanged<T>;

    using PowerManager = embr::esp_idf::service::v1::PowerManager;
    using Timer = embr::esp_idf::service::v1::GPTimer;

    void on_notify(changed<
        embr::Service::id::config<const gptimer_config_t&> > e)
    {
        ESP_LOGI(TAG, "timer configured: resolution_hz=%" PRIu32,
            e.value.resolution_hz);
    }

    void on_notify(Timer::event::callback);

    template <class Period>
    void on_notify(const TimerFilterService<Period>::event::callback&);

    void on_notify(changed<embr::Service::id::substate> e, PowerManager&)
    {
        if(e.value == embr::Service::Waking)
            ESP_LOGI(TAG, "bringup");
        else if(e.value == embr::Service::Sleeping)
            ESP_LOGI(TAG, "sleeping");
    }
};
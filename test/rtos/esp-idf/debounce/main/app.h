#include <estd/port/freertos/queue.h>

#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/gptimer.h>
#include <embr/platform/esp-idf/service/pm.h>


#include "debounce.h"

struct App
{
    static constexpr const char* TAG = "App";

    estd::freertos::layer1::queue<Item, 5> q;

    using Timer = embr::esp_idf::service::v1::GPTimer;

    void on_notify(Timer::event::callback);
};
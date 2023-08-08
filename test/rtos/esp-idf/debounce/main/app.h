#include <estd/port/freertos/queue.h>

#include <embr/service.h>

#include <embr/platform/esp-idf/debounce2.h>
#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/gptimer.h>


struct App
{
    static constexpr const char* TAG = "App";

    using Event = embr::debounce::v1::Event;
    using Timer = embr::esp_idf::service::v1::GPTimer;

    estd::freertos::layer1::queue<Event, 5> q;

    void on_notify(Timer::event::callback);
};
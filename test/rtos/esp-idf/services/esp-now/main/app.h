#include <estd/port/freertos/queue.h>
#include <estd/internal/variant/storage.h>  // DEBT: In meantime until layer1::queue is improved...

#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/esp-now.h>
#include <embr/platform/esp-idf/service/event.h>


struct App
{
    static constexpr const char* TAG = "App";

    using EspNow = embr::esp_idf::service::v1::EspNow;

    void on_notify(EspNow::event::receive);
    void on_notify(EspNow::event::send);
};
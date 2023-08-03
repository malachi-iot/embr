#include <estd/port/freertos/queue.h>
#include <estd/internal/variant/storage.h>  // DEBT: In meantime until layer1::queue is improved...

#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/pm.h>
#include <embr/platform/esp-idf/service/twai.h>


struct App
{
    static constexpr const char* TAG = "App";

    using TWAI = embr::esp_idf::service::v1::TWAI;

    void on_notify(TWAI::event::alert);
};
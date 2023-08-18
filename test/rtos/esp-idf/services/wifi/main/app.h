#include <estd/port/freertos/queue.h>
#include <estd/internal/variant/storage.h>  // DEBT: In meantime until layer1::queue is improved...

#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/pm.h>
#include <embr/platform/esp-idf/service/wifi.h>


struct App
{
    static constexpr const char* TAG = "App";

    using WiFi = embr::esp_idf::service::v1::WiFi;

    void on_notify(WiFi::event::wifi<WIFI_EVENT_STA_CONNECTED>);
    void on_notify(WiFi::event::ip<IP_EVENT_STA_GOT_IP>);
};
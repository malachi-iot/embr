#include <estd/port/freertos/queue.h>
#include <estd/internal/variant/storage.h>  // DEBT: In meantime until layer1::queue is improved...

#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/pm.h>
#include <embr/platform/esp-idf/service/wifi.h>


struct App
{
    static constexpr const char* TAG = "App";

    bool got_ip = false;

    using WiFi = embr::esp_idf::service::v1::WiFi;

    void on_notify(WiFi::event::wifi<WIFI_EVENT_STA_CONNECTED>);
    void on_notify(WiFi::event::ip<IP_EVENT_STA_GOT_IP>);
    void on_notify(WiFi::event::ip<IP_EVENT_STA_LOST_IP>)
    {
        got_ip = false;
    }

#if __cpp_nontype_template_parameter_auto
    void on_notify(embr::esp_idf::event::v2::base<WIFI_EVENT_STA_START>);
#endif
};
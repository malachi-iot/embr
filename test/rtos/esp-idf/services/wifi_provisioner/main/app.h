#include <estd/port/freertos/queue.h>
#include <estd/internal/variant/storage.h>  // DEBT: In meantime until layer1::queue is improved...

#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/event.h>
#include <embr/platform/esp-idf/service/pm.h>
#include <embr/platform/esp-idf/service/wifi_provisioner.h>


struct App
{
    static constexpr const char* TAG = "App";

    static constexpr const char ep3[] = "test3";

    void on_notify(embr::esp_idf::event::v1::wifi_prov<WIFI_PROV_CRED_FAIL>);
    void on_notify(embr::esp_idf::event::v1::wifi_prov<WIFI_PROV_CRED_RECV>);
};
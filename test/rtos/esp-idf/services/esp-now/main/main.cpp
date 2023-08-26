#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"

#include <esp_wifi_types.h>

#include <estd/chrono.h>
#include <estd/optional.h>
#include <estd/thread.h>

#include <embr/platform/freertos/service/worker.hpp>

#include <embr/platform/esp-idf/service/diagnostic.h>
#include <embr/platform/esp-idf/service/event.hpp>
#include <embr/platform/esp-idf/service/esp-now.hpp>
#include <embr/platform/esp-idf/service/flash.hpp>
#include <embr/platform/esp-idf/service/wifi.hpp>

// Adapted from
// https://github.com/espressif/esp-idf/blob/v5.1.1/examples/wifi/espnow/main/espnow_example_main.c


namespace service = embr::esp_idf::service::v1;
using Diagnostic = service::Diagnostic;

#include "app.h"


namespace app_domain {

App app;

typedef estd::integral_constant<App*, &app> app_singleton;
using tier2 = embr::layer0::subject<Diagnostic, app_singleton>;

using esp_now_type = service::EspNow::static_type<tier2>;
using wifi_type = service::WiFi::static_type<tier2>;

using tier1 = tier2::append<esp_now_type, wifi_type>;

embr::freertos::worker::Service::runtime<tier1> worker(512);

}


using namespace estd::chrono_literals;

#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_IF        ESP_IF_WIFI_STA
#define ESPNOW_WIFI_IF   WIFI_IF_STA


extern "C" void app_main()
{
    constexpr const char* TAG = "app_main";

    service::Flash::runtime<app_domain::tier1>{}.start();
    service::EventLoop::runtime<app_domain::tier1>{}.start();
    service::NetIf::runtime<app_domain::tier1>{}.start();

    app_domain::wifi_type::value->config(ESPNOW_WIFI_MODE);
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    app_domain::wifi_type::value->start();

    // DEBT: Document exactly why we do this, but pretty sure it's because ESP-NOW
    // doesn't attempt to hop challens and WiFi does
    ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));
    // Sets long range mode at reduced speed
    ESP_ERROR_CHECK(esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );

    app_domain::esp_now_type::value->start();

    app_domain::worker.start();

    app_domain::worker << [] { ESP_LOGI(TAG, "From worker!"); };

    for(;;)
    {
        static int counter = 0;

        ESP_LOGI(TAG, "counting: %d", ++counter);

        estd::this_thread::sleep_for(5s);
    }
}


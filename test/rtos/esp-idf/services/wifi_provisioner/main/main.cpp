#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include <wifi_provisioning/scheme_ble.h>

#include <estd/chrono.h>
#include <estd/optional.h>
#include <estd/thread.h>

#include <embr/platform/esp-idf/service/diagnostic.h>
#include <embr/platform/esp-idf/service/flash.hpp>
#include <embr/platform/esp-idf/service/event.hpp>
#include <embr/platform/esp-idf/service/wifi_provisioner.hpp>


namespace service = embr::esp_idf::service::v1;
using Diagnostic = service::Diagnostic;

#include "app.h"

namespace app_domain {

App app;

typedef estd::integral_constant<App*, &app> app_singleton;
using tier2 = embr::layer0::subject<Diagnostic, app_singleton>;

using tier1 = tier2;

}


using namespace estd::chrono_literals;


extern "C" void app_main()
{
    const char* TAG = "app_main";

    service::Flash::runtime<app_domain::tier1>{}.start();
    service::EventLoop::runtime<app_domain::tier1>{}.start();

    service::WiFiProvisioner::runtime<app_domain::tier2> provisioner;

#ifndef CONFIG_BT_ENABLED
#error This example requires BLE
#endif

    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,

        // NOTE: Deprecated
        .app_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
    };

    provisioner.start(config);

    for(;;)
    {
        static int counter = 0;

        ESP_LOGI(TAG, "counting: %d", ++counter);

        estd::this_thread::sleep_for(5s);
    }
}


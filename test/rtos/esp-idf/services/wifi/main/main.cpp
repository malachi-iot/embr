#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include <estd/chrono.h>
#include <estd/optional.h>
#include <estd/thread.h>

#include <embr/platform/esp-idf/service/diagnostic.h>
#include <embr/platform/esp-idf/service/flash.hpp>
#include <embr/platform/esp-idf/service/wifi.hpp>


using Diagnostic = embr::esp_idf::service::v1::Diagnostic;

#include "app.h"


namespace app_domain {

App app;

typedef estd::integral_constant<App*, &app> app_singleton;
typedef embr::layer0::subject<Diagnostic, app_singleton> filter_observer;

using flash_type = embr::esp_idf::service::v1::Flash::runtime<filter_observer>;
using wifi_type = embr::esp_idf::service::v1::WiFi::runtime<filter_observer>;

wifi_type wifi;

}


using namespace estd::chrono_literals;


extern "C" void app_main()
{
    const char* TAG = "app_main";

    app_domain::flash_type{}.start();

    // DEBT: Putting this guy down here because we don't have an
    // event loop initialization service just yet
    esp_netif_t* wifi_netif = app_domain::wifi.create_default();

    wifi_config_t wifi_config = {};

    strcpy((char*)wifi_config.sta.ssid, CONFIG_WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, CONFIG_WIFI_PASSWORD);

    // err=0x1101 means NVS fail
    app_domain::wifi.start(WIFI_MODE_STA, &wifi_config);

    for(;;)
    {
        static int counter = 0;

        ESP_LOGI(TAG, "counting: %d", ++counter);

        estd::this_thread::sleep_for(5s);
    }
}


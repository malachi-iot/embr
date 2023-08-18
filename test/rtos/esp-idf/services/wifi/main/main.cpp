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

// NOTE: These events already are logged by way of esp-idf's system, but we do it
// again just to demonstrate app ability to hook events

void App::on_notify(WiFi::event::wifi<WIFI_EVENT_STA_CONNECTED>)
{
    ESP_LOGI(TAG, "on_notify: WiFi connected to AP");
}


void App::on_notify(WiFi::event::ip<IP_EVENT_STA_GOT_IP> e)
{
    char buf[32];

    ESP_LOGI(TAG, "on_notify: got ip: %s",
        esp_ip4addr_ntoa(&e->ip_info.ip, buf, sizeof(buf)));
}

#if __cpp_nontype_template_parameter_auto
void App::on_notify(embr::esp_idf::event::v2::base<WIFI_EVENT_STA_START>)
{
    ESP_LOGI(TAG, "on_notify: WiFi started");
}
#endif



namespace app_domain {

App app;

typedef estd::integral_constant<App*, &app> app_singleton;
typedef embr::layer0::subject<Diagnostic, app_singleton> filter_observer;

using wifi_type = embr::esp_idf::service::v1::WiFi::static_type<filter_observer>;

using layer1 = filter_observer::append<wifi_type>;

using flash_type = embr::esp_idf::service::v1::Flash::runtime<layer1>;
using event_loop = embr::esp_idf::service::v1::EventLoop::runtime<layer1>;

}


using namespace estd::chrono_literals;


extern "C" void app_main()
{
    const char* TAG = "app_main";

    app_domain::flash_type{}.start();
    app_domain::event_loop{}.start();

    // DEBT: Putting this guy down here because we don't have an
    // event loop initialization service just yet
    app_domain::wifi_type::value->create_default_sta();

    wifi_config_t wifi_config = {};

    strcpy((char*)wifi_config.sta.ssid, CONFIG_WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, CONFIG_WIFI_PASSWORD);

    // err=0x1101 means NVS fail
    app_domain::wifi_type::value->start(WIFI_MODE_STA, &wifi_config);

    for(;;)
    {
        static int counter = 0;

        ESP_LOGI(TAG, "counting: %d", ++counter);

        estd::this_thread::sleep_for(5s);
    }
}


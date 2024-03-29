#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include <wifi_provisioning/scheme_ble.h>
#include <wifi_provisioning/scheme_console.h>

#include <estd/chrono.h>
#include <estd/optional.h>
#include <estd/thread.h>

#include <embr/platform/esp-idf/service/diagnostic.h>
#include <embr/platform/esp-idf/service/flash.hpp>
#include <embr/platform/esp-idf/service/event.hpp>
#include <embr/platform/esp-idf/service/wifi.hpp>
#include <embr/platform/esp-idf/service/wifi_provisioner.hpp>


namespace service = embr::esp_idf::service::v1;
using Diagnostic = service::Diagnostic;

#include "app.h"


void App::on_notify(embr::esp_idf::event::v1::wifi_prov<WIFI_PROV_CRED_FAIL> e)
{
    // NOTE: Be advised, provisioner doesn't automatically give you another
    // chance if you fail here.  I speculate this is to mitigate attacks,
    // giving the programmer explicit control over how often to let people
    // attempt provisioning.
    ESP_LOGW(TAG, "on_notify: failed with reason %d", *e.data);
}


void App::on_notify(embr::esp_idf::event::v1::wifi_prov<WIFI_PROV_CRED_RECV> e)
{
    ESP_LOGI(TAG, "on_notify: got credentials");
}


void App::on_notify(embr::esp_idf::event::v1::ip<IP_EVENT_STA_GOT_IP> e)
{
    ESP_LOGI(TAG, "on_notify: ip address=" IPSTR, IP2STR(&e->ip_info.ip));
}


namespace app_domain {

App app;

typedef estd::integral_constant<App*, &app> app_singleton;
using tier2 = embr::layer0::subject<Diagnostic, app_singleton>;

using wifi_type = service::WiFi::static_type<tier2>;

using tier1 = tier2::append<wifi_type>;

}


#ifndef CONFIG_BT_ENABLED
#error This example requires BLE
#endif


using namespace estd::chrono_literals;

// Copied directly from wifi_provisioner example
static void prep_ble()
{
    /* This step is only useful when scheme is wifi_prov_scheme_ble. This will
     * set a custom 128 bit UUID which will be included in the BLE advertisement
     * and will correspond to the primary GATT service that provides provisioning
     * endpoints as GATT characteristics. Each GATT characteristic will be
     * formed using the primary service UUID as base, with different auto assigned
     * 12th and 13th bytes (assume counting starts from 0th byte). The client side
     * applications must identify the endpoints by reading the User Characteristic
     * Description descriptor (0x2901) for each characteristic, which contains the
     * endpoint name of the characteristic */
    uint8_t custom_service_uuid[] = {
    /* LSB <---------------------------------------
     * ---------------------------------------> MSB */
    0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
    0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
    };

    /* If your build fails with linker errors at this point, then you may have
     * forgotten to enable the BT stack or BTDM BLE settings in the SDK (e.g. see
     * the sdkconfig.defaults in the example project) */
    wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);
}


extern "C" void app_main()
{
    const char* TAG = "app_main";

    service::Flash::runtime<app_domain::tier1>{}.start();
    service::EventLoop::runtime<app_domain::tier1>{}.start();

    service::WiFiProvisioner::runtime<app_domain::tier2> provisioner;

    // DEBT: Awkwardness here.  Do we make a standalone netif service?
    ESP_ERROR_CHECK(esp_netif_init());

    // Minimal config just to get wifi radios online.  We let provisioner
    // do its magic from there
    app_domain::wifi_type::value->config();

    // wifi provisioner can only do one scheme at a time, bummer
    //service::WiFiProvisioner::runtime<app_domain::tier2> provisioner_console;

    // Optional, this effectively bounces BLE events through provisioner upward
    // through tier2
    service::EventLoop::handler_register<PROTOCOMM_TRANSPORT_BLE_EVENT>(
        &provisioner);

    /* Do we want a proof-of-possession (ignored if Security 0 is selected):
    *      - this should be a string with length > 0
    *      - NULL if not used
    */
    const char *pop = "abcd1234";

    /* This is the structure for passing security parameters
    * for the protocomm security 1.
    */
    wifi_prov_security1_params_t *sec_params = pop;

    const char* service_name = "PROV_service";

    provisioner.config(
        wifi_prov_scheme_ble,
        WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM);

    prep_ble();

    provisioner.start(WIFI_PROV_SECURITY_1, (const void *) sec_params, service_name);

    for(;;)
    {
        static int counter = 0;

        ESP_LOGI(TAG, "counting: %d", ++counter);

        estd::this_thread::sleep_for(5s);
    }
}


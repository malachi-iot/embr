#include <string.h>

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include <estd/internal/platform.h>

#include "esp-helper.h"

// lifting from my own user_main and from
// https://github.com/espressif/esp-idf/blob/v3.3/examples/wifi/getting_started/station/main/station_example_main.c
// https://github.com/espressif/esp-idf/blob/v4.0-beta2/examples/wifi/getting_started/station/main/station_example_main.c
#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
void event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    static const char* TAG = "event_handler";
}
#else
esp_err_t event_handler(void* ctx, system_event_t* event)
{
    static int station_retry_num = 0;
    static const char* TAG = "event_handler";

    switch(event->event_id)
    {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;

        case SYSTEM_EVENT_STA_GOT_IP:
            // doing LOGD since global system event fires off an
            // informational level ip report log
            ESP_LOGD(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            station_retry_num = 0;
            break;


        case SYSTEM_EVENT_STA_DISCONNECTED:
            if(station_retry_num++ < 5)
            {
                ESP_LOGI(TAG, "STA_DISCONNECTED: Retrying");
                esp_wifi_connect();
            }
            else
                ESP_LOGW(TAG, "STA_DISCONNECTED: Giving up");

            break;

        default:
            break;
    }
    return ESP_OK;
}
#endif

void init_flash()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

// largely copy/paste from
// https://github.com/espressif/esp-idf/blob/v3.3/examples/wifi/getting_started/station/main/station_example_main.c
#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
void wifi_init_sta()
#else
void wifi_init_sta(system_event_cb_t event_handler)
#endif
{
    static const char *TAG = "wifi_init_sta";

    tcpip_adapter_init();
#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
    ESP_LOGI(TAG, "create default event loop");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
#else
    ESP_LOGI(TAG, "init event loop");
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
#endif

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    // https://esp32.com/viewtopic.php?t=1317
    // for ssid/password config via C++
    wifi_config_t wifi_config = {};

    // TODO: May do these elsewhere once everything is organized
#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
#endif

    strcpy((char*)wifi_config.sta.ssid, CONFIG_WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, CONFIG_WIFI_PASSWORD);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
             CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);
}

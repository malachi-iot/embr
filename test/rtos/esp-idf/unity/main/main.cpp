#include <stdio.h>
#include <string.h>
#include "unity.h"

#include <embr/streambuf.h>
#include <estd/internal/platform.h>

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

static void print_banner(const char* text);

// even though 'esp_event_loop_create_default' exists in previous
// versions, something appears to be off about it
// https://www.gitmemory.com/issue/espressif/esp-idf/3838/515464313
// https://github.com/espressif/esp-idf/issues/3838
// https://github.com/espressif/esp-idf/issues/668
// so until that's all worked out, we feature flag it
#if ESTD_IDF_VER >= ESTD_IDF_VER_4_0_0
#define FEATURE_IDF_DEFAULT_EVENT_LOOP
#endif

// lifting from my own user_main and from
// https://github.com/espressif/esp-idf/blob/v3.3/examples/wifi/getting_started/station/main/station_example_main.c
// https://github.com/espressif/esp-idf/blob/v4.0-beta2/examples/wifi/getting_started/station/main/station_example_main.c
#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
static void event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{

}
#else
static esp_err_t event_handler(void* ctx, system_event_t* event)
{
    return ESP_OK;
}
#endif

// largely copy/paste from
// https://github.com/espressif/esp-idf/blob/v3.3/examples/wifi/getting_started/station/main/station_example_main.c
void wifi_init_sta()
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
    wifi_config_t wifi_config;

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

extern "C" void app_main()
{
    static const char *TAG = "app_main";

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    // --------

    print_banner("Running all the registered tests");
    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();

    print_banner("Starting interactive test menu");
    /* This function will not return, and will be busy waiting for UART input.
     * Make sure that task watchdog is disabled if you use this function.
     */
    unity_run_menu();
}

static void print_banner(const char* text)
{
    printf("\n#### %s #####\n\n", text);
}

#include <esp_log.h>
#include <esp_mac.h>
#include <esp_netif.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

#include <embr/exp/v4/retry.h>

// Lots of code lifted from
// https://github.com/espressif/esp-idf/blob/v5.4.1/examples/wifi/espnow/

const char* TAG = "embr::test::retry";

#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA

static uint8_t broadcast_mac[] { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{

}

static void recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    if(esp_now_is_peer_exist(recv_info->src_addr) == false)
    {
        esp_now_peer_info_t peer{};

        peer.channel = CONFIG_ESPNOW_CHANNEL;
        peer.ifidx = (wifi_interface_t)ESPNOW_WIFI_IF;
        peer.encrypt = false;
        memcpy(peer.peer_addr, recv_info->src_addr, ESP_NOW_ETH_ALEN);
        ESP_ERROR_CHECK(esp_now_add_peer(&peer));
    }
}

static void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    const wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start());
    ESP_ERROR_CHECK( esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif
}


static void espnow_init()
{
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(recv_cb) );
#if CONFIG_ESPNOW_ENABLE_POWER_SAVE
    ESP_ERROR_CHECK( esp_now_set_wake_window(CONFIG_ESPNOW_WAKE_WINDOW) );
    ESP_ERROR_CHECK( esp_wifi_connectionless_module_set_wake_interval(CONFIG_ESPNOW_WAKE_INTERVAL) );
#endif
    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );    

    esp_now_peer_info_t peer{};

    peer.channel = CONFIG_ESPNOW_CHANNEL;
    peer.ifidx = (wifi_interface_t)ESPNOW_WIFI_IF;
    peer.encrypt = false;
    memcpy(peer.peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));
}


extern "C" void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    wifi_init();
    espnow_init();
}

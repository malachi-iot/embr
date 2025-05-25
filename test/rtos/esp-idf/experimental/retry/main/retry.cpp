#include <esp_log.h>
#include <esp_mac.h>
#include <esp_netif.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

#include <embr/exp/v4/retry.h>

#include "retry.h"

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

using namespace embr::experimental;
using endpoint_type = estd::array<uint8_t, 6>;
using hasher = estd::internal::container_hash<uint32_t>;
using tracked_type = estd::array<uint8_t, 250>;
using retry_type = v4::Retry<v4::RetryImpl<10, endpoint_type, tracked_type, hasher>>;
using clock_type = retry_type::clock_type;
using pointer = retry_type::pointer;

retry_type retry;

extern "C" void app_main(void)
{
    using namespace std::chrono_literals;

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

    const endpoint_type ep = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    pointer tracked = retry.track(ep, clock_type::now() + 250ms);
    auto pkt = new (&tracked->second) packet;

    pkt->seq = 123;
    pkt->ack = 0;

    ESP_ERROR_CHECK(esp_now_send(ep.data(), tracked->second.data(), sizeof(packet)));

    for(;;)
    {
        vTaskDelay(pdMS_TO_TICKS(250));
        retry.poll_one(clock_type::now(), [&](pointer p)
        {
            if(p->second.attempt_count_ > 5)
            {
                ESP_LOGI(TAG, "giving up");
                return false;
            }

            p->second.next_attempt_ += (2 + p->second.attempt_count_) * 500ms;

            ESP_LOGI(TAG, "retry polling");
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, p->first.data(), 6, ESP_LOG_INFO);
            ESP_ERROR_CHECK(esp_now_send(ep.data(), p->second.data(), sizeof(packet)));

            return true;
        });
    }
}

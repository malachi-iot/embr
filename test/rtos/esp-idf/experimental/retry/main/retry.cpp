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

using namespace embr::experimental;

using mac_type = estd::array<uint8_t, 6>;

struct endpoint_type
{
    uint16_t mid;       // CoAP style
    mac_type mac;

    constexpr endpoint_type() : mid{}, mac{} {}

    constexpr endpoint_type(uint16_t mid, mac_type mac) :
        mid{mid},
        mac{mac}
    {}

    constexpr bool operator==(const endpoint_type& other) const
    {
        return mac == other.mac;
        //return mid == other.mid && mac == other.mac;
    }

    // packed important so that hash behaves itself, not just a size thing
//}   __attribute__((packed));
};

namespace estd {

// TODO: Consdider a brute-force container_hash and/or a container_hash_tag for things like
// endpoint_type to derive from
template <>
struct hash<endpoint_type>
{
    size_t operator()(const endpoint_type& v) const
    {
        //auto buf = reinterpret_cast<const uint8_t*>(&v);
        //return estd::internal::fnv_hash<uint32_t>::hash(buf, buf + sizeof(endpoint_type));

        // Due to https://github.com/malachi-iot/estdlib/issues/116, only comparing mac
        // (hash at risk of computing wrong)
        return estd::internal::container_hash{}(v.mac);
    }
};

}

using tracked_type = estd::array<uint8_t, 250>;
using retry_type = v4::Retry<v4::RetryImpl<10, endpoint_type, tracked_type>>;
using clock_type = retry_type::clock_type;
using pointer = retry_type::pointer;

bool discoved = false;
retry_type retry;
endpoint_type buddy;

static void send(const endpoint_type& ep, const packet* p)
{
    ESP_ERROR_CHECK(esp_now_send(ep.mac.data(), (const uint8_t*)p, sizeof(packet)));
}


void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    ESP_LOGD(TAG, "send_cb");

}

void recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    ESP_LOGI(TAG, "recv_cb: len=%d", len);
    const uint8_t* src_addr = recv_info->src_addr;
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, src_addr, 6, ESP_LOG_INFO);

    if(esp_now_is_peer_exist(src_addr) == false)
    {
        ESP_LOGI(TAG, "recv_cb: adding peer");
        esp_now_peer_info_t peer{};

        peer.channel = CONFIG_ESPNOW_CHANNEL;
        peer.ifidx = (wifi_interface_t)ESPNOW_WIFI_IF;
        peer.encrypt = false;
        memcpy(peer.peer_addr, src_addr, ESP_NOW_ETH_ALEN);
        ESP_ERROR_CHECK(esp_now_add_peer(&peer));
    }

    auto p = (const packet*) data;

    mac_type mac;
    estd::copy_n(src_addr, 6, mac.begin());

    if(p->announce)
    {
        ESP_LOGI(TAG, "recv_cb: announce received");
        discoved = true;

        buddy.mid = p->seq;
        buddy.mac = mac;
    }
    else if(p->ack)
    {
        ESP_LOGI(TAG, "recv_cb: ACK received");
        retry.ack_received(endpoint_type(p->seq, mac));
    }
    else
    {
        ESP_LOGI(TAG, "recv_cb: data packet %d, sending ACK out", p->seq);

        packet reply = *p;
        
        reply.ack = 1;

        send(endpoint_type(p->seq, mac), &reply);
    }
}

void wifi_init();
void espnow_init();

extern "C" void app_main()
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

    const endpoint_type ep = {0, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

    ESP_LOGI(TAG, "Looking for buddy...");

    // TODO: Announce phase has issues when PM is active, probably because announce packets don't go through an ACK
    // procedure
    while(!discoved)
    {
        packet disco;

        disco.announce = 1;

        vTaskDelay(pdMS_TO_TICKS(2000));
        send(ep, &disco);
    }

    ESP_LOGI(TAG, "Found my buddy!");

    pointer tracked = retry.track(buddy, clock_type::now() + 250ms);

    if(tracked == nullptr)
    {
        ESP_LOGE(TAG, "main: track failed!");
        return;
    }

    auto pkt = new (&tracked->second) packet;

    pkt->seq = 123;
    pkt->ack = 0;

    send(buddy, pkt);

    for(;;)
    {
        vTaskDelay(pdMS_TO_TICKS(250));
        retry.poll(clock_type::now(), [](pointer p)
        {
            if(p->second.attempt_count_ > 5)
            {
                ESP_LOGI(TAG, "giving up");
                return false;
            }

            p->second.next_attempt_ += (2 + p->second.attempt_count_) * 500ms;

            ESP_LOGI(TAG, "retry polling");
            //ESP_LOG_BUFFER_HEX_LEVEL(TAG, p->first.mac.data(), 6, ESP_LOG_INFO);
            //send(p->first, &p->second);
            ESP_ERROR_CHECK(esp_now_send(p->first.mac.data(), p->second.data(), sizeof(packet)));

            return true;
        });
    }
}

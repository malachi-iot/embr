#include <random>

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
        return mid == other.mid && mac == other.mac;
    }

    // packed important so that hash behaves itself, not just a size thing
}   __attribute__((packed));

namespace estd {

// TODO: Consdider a brute-force container_hash and/or a container_hash_tag for things like
// endpoint_type to derive from
template <>
struct hash<endpoint_type>
{
    size_t operator()(const endpoint_type& v) const
    {
        auto buf = reinterpret_cast<const uint8_t*>(&v);
        return estd::internal::fnv_hash<uint32_t>::hash(buf, buf + sizeof(endpoint_type));
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
TaskHandle_t self_task;

std::random_device r;
 
std::default_random_engine e1(r());
std::uniform_int_distribution<unsigned> uniform_dist(0, 100);
constexpr unsigned msg_loss_thresh = 100 - CONFIG_RETRY_SEND_LOSSINESS;
constexpr unsigned ack_loss_thresh = 100 - CONFIG_RETRY_ACK_LOSSINESS;

static bool send(const endpoint_type& ep, const packet* p,
    unsigned loss_thresh = 100, const char* desc = "generic")
{
    unsigned rnd;

    if((rnd = uniform_dist(e1)) <= loss_thresh)
    {
        ESP_ERROR_CHECK(esp_now_send(ep.mac.data(), (const uint8_t*)p, sizeof(packet)));
        return true;
    }
    else
    {
        ESP_LOGW(TAG, "send: synthetic drop %s (rnd=%u)", desc, rnd);
    }

    return false;
}

static void send_ack(const endpoint_type& ep, const packet* p)
{
    send(ep, p, ack_loss_thresh, "ACK");
}

static void send_msg(const endpoint_type& ep, const packet* p)
{
    send(ep, p, msg_loss_thresh, "message");
}



void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    ESP_LOGD(TAG, "send_cb");

}

void recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    auto p = (const packet*) data;

    ESP_LOGD(TAG, "recv_cb: len=%d, seq=%d", len, p->seq);
    const uint8_t* src_addr = recv_info->src_addr;

    if(esp_now_is_peer_exist(src_addr) == false)
    {
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, src_addr, 6, ESP_LOG_INFO);
        ESP_LOGI(TAG, "recv_cb: adding peer");
        esp_now_peer_info_t peer{};

        peer.channel = CONFIG_ESPNOW_CHANNEL;
        peer.ifidx = (wifi_interface_t)ESPNOW_WIFI_IF;
        peer.encrypt = false;
        memcpy(peer.peer_addr, src_addr, ESP_NOW_ETH_ALEN);
        ESP_ERROR_CHECK(esp_now_add_peer(&peer));
    }
    else
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, src_addr, 6, ESP_LOG_DEBUG);

    const mac_type& mac = *(mac_type*)src_addr;

    if(p->announce)
    {
        if(!discoved) 
            ESP_LOGI(TAG, "recv_cb: announce received (discovered)");
        else
            ESP_LOGD(TAG, "recv_cb: announce received");

        discoved = true;

        buddy.mac = mac;
    }
    else if(p->ack)
    {
        bool matched_tracked = retry.ack_received(endpoint_type(p->seq, mac));
        ESP_LOGI(TAG, "recv_cb: ACK received (matched=%u)", matched_tracked);

        // NOTE: In a full application, notify only when ack_received matches with 'top' (tracking multiple
        // sends).  We only do one send, so it doesn't matter at the moment.
        xTaskNotifyIndexed(self_task, 0, 1, eSetBits);
    }
    else
    {
        ESP_LOGI(TAG, "recv_cb: data packet seq=%d, sending ACK out", p->seq);

        packet reply = *p;
        
        reply.ack = 1;

        send_ack(endpoint_type(p->seq, mac), &reply);
    }
}

void wifi_init();
void espnow_init();

static void loop()
{
    using namespace std::chrono_literals;

    constexpr endpoint_type ep = {0, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

    ESP_LOGI(TAG, "Looking for buddy...");

    discoved = false;

    // TODO: Announce phase has issues when PM is active, probably because announce packets don't go through an ACK
    // procedure
    while(!discoved)
    {
        packet disco;

        disco.announce = 1;

        send(ep, &disco);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "Found my buddy! Sending a packet");

    buddy.mid++;
    pointer tracked = retry.track(buddy, clock_type::now() + 250ms);

    if(tracked == nullptr)
    {
        ESP_LOGE(TAG, "main: track failed!");
        return;
    }

    auto pkt = new (&tracked->second) packet(buddy.mid);

    send_msg(buddy, pkt);

    // Q: Race condition with ack_received?  Maybe.  Let's be sure
    while(!retry.empty())
    {
        pointer top = retry.top();
        clock_type::time_point next = top->second.next_attempt();
        auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(next - clock_type::now());
        auto interval_ms = std::max(0LL, interval.count());

        // + 1 more or less rounds up, + 1 again for fudge factor
        TickType_t interval_ticks = pdMS_TO_TICKS(interval_ms) + 2;
        //vTaskDelay(pdMS_TO_TICKS(250));

        uint32_t ack_received = 0;
        BaseType_t r = xTaskNotifyWaitIndexed(0, 0, 1, &ack_received, interval_ticks);

        if(r == pdFALSE)
            ESP_LOGD(TAG, "Natural timeout waiting for ACK (none received)");
        else if(ack_received)
            ESP_LOGI(TAG, "app_main: ACK detected");

        int processed = retry.poll(clock_type::now(), [](pointer p)
        {
            if(p->second.attempt_count() > 5)
            {
                ESP_LOGI(TAG, "giving up");
                return false;
            }

            p->second.next_attempt_ += (2 + p->second.attempt_count()) * 500ms;

            ESP_LOGI(TAG, "retry polling");
            //ESP_LOG_BUFFER_HEX_LEVEL(TAG, p->first.mac.data(), 6, ESP_LOG_INFO);
            send_msg(p->first, (const packet*)&p->second);

            return true;
        });

        if(processed == 0 && !ack_received)
            ESP_LOGW(TAG, "Always expect one item to process here (interval=%ums)",
                (unsigned)interval.count());
    }

    ESP_LOGI(TAG, "Delay ----");
    vTaskDelay(pdMS_TO_TICKS(15000));
}

extern "C" void app_main()
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    self_task = xTaskGetCurrentTaskHandle();

    wifi_init();
    espnow_init();

    for(;;) loop();
}

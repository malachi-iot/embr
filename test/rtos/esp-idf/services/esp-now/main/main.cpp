#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"

#include <esp_wifi_types.h>

#include <estd/chrono.h>
#include <estd/optional.h>
#include <estd/thread.h>
#include <estd/sstream.h>

#include <embr/platform/freertos/service/worker.hpp>

#include <embr/platform/esp-idf/service/diagnostic.h>
#include <embr/platform/esp-idf/service/event.hpp>
#include <embr/platform/esp-idf/service/esp-now.hpp>
#include <embr/platform/esp-idf/service/flash.hpp>
#include <embr/platform/esp-idf/service/wifi.hpp>

#include <embr/platform/esp-idf/board.h>

// Adapted from
// https://github.com/espressif/esp-idf/blob/v5.1.1/examples/wifi/espnow/main/espnow_example_main.c


namespace service = embr::esp_idf::service::v1;
using Diagnostic = service::Diagnostic;
using namespace estd::chrono_literals;
using board_traits = embr::esp_idf::board_traits;

#include "app.h"


namespace app_domain {

App app;

typedef estd::integral_constant<App*, &app> app_singleton;
using tier2 = embr::layer0::subject<Diagnostic, app_singleton>;

using esp_now_type = service::EspNow::static_type<tier2>;
using wifi_type = service::WiFi::static_type<tier2>;

using tier1 = tier2::append<esp_now_type, wifi_type>;

embr::freertos::service::v1::Worker::runtime<tier1> worker(512);

}

static esp_now_peer_info_t broadcast_peer {};
static constexpr uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };


// Intermediate/experimental flavors - not in use
void on_notify_alternate(service::EspNow::event::receive e)
{
    // DEBT: Bigtime debt, we double-copy the max-250 buffer -
    // one time into temp, then a 2nd time when putting it into
    // worker.  Would be preferable to use ring buffer directly,
    // or to extend worker to take a size parameter for "extra" data
    // per enqueue
    uint8_t temp[250];
    unsigned sz = e.data.size();

    memcpy(temp, e.data.data(), sz);

    app_domain::worker << [temp] {};

    // Very similar to function bind except this one works with runtime
    // sized storage
    app_domain::worker.queue.enqueue_with_storage([]
        (const uint8_t* buf, unsigned sz2)
        {

        }, portMAX_DELAY, e.data.data(), sz);
}
void init_alternate()
{
    static constexpr const char* TAG = "init_alternate";

    app_domain::worker << [] { ESP_LOGI(TAG, "From worker!"); };
    // Actually works, but I went for second ringbuffer with the idea that it could
    // operate in split mode
    app_domain::worker.queue.enqueue_with_storage([](const uint8_t* data, unsigned sz)
    {
        ESP_LOGI(TAG, "From worker2! %s", (const char*)data);
    }, portMAX_DELAY, (const uint8_t*) "Hello", 6);
}


// TODO: Put this into ring_buffer wrapper itself
void App::on_notify(EspNow::event::receive e)
{
    // 50ms arbitrarily chosen
    BaseType_t r = ring.emplace_add_size<EspNow::recv_info>(
        pdMS_TO_TICKS(50), e.data.size(), e);

    if(r == pdFALSE)
    {
        ESP_LOGW(TAG, "Error double-buffering into ring buffer");
        return;
    }

    // NOTE: Just doing logging, probably quick enough to do without worker
    // but this is a good reference for real world hand-off
    app_domain::worker << [this]
    {
        size_t sz;
        auto rx = (EspNow::recv_info*)ring.receive(&sz, 0);

        if(rx != nullptr)
        {
            sz -= sizeof(EspNow::recv_info);
            const uint8_t* source = rx->source;

            ESP_LOGI(TAG, "rx: src mac=" MACSTR ", rssi=%d, noise_floor=%d",
                MAC2STR(source), rx->rx_ctrl.rssi, rx->rx_ctrl.noise_floor);
            ESP_LOG_BUFFER_HEXDUMP(TAG, rx->data, sz, ESP_LOG_INFO);

            ring.return_item(rx);
        }
        else
            ESP_LOGW(TAG, "data always expected here");
    };
}


void App::on_notify(EspNow::event::send e)
{
    // DEBT: Not sure if MAC needs to be copied here, I don't think so
    // actually since esp_now_send *seems* to need the peer registered first
    EspNow::send_info proxied(e);

    app_domain::worker << [proxied]
    {
    };
}



#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_IF        ESP_IF_WIFI_STA
#define ESPNOW_WIFI_IF   WIFI_IF_STA
#define ESPNOW_CHANNEL   1

extern "C" void app_main()
{
    constexpr const char* TAG = "app_main";

    service::Flash::runtime<app_domain::tier1>{}.start();
    service::EventLoop::runtime<app_domain::tier1>{}.start();
    service::NetIf::runtime<app_domain::tier1>{}.start();

    app_domain::wifi_type::value->config(ESPNOW_WIFI_MODE, WIFI_STORAGE_RAM);
    app_domain::wifi_type::value->start();

    // DEBT: Document exactly why we do this, but pretty sure it's because ESP-NOW
    // doesn't attempt to hop challens and WiFi does
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    // Sets long range mode at reduced speed
    ESP_ERROR_CHECK(esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );

    app_domain::esp_now_type::value->start();

    app_domain::worker.start();

    broadcast_peer.channel = ESPNOW_CHANNEL;
    broadcast_peer.ifidx = ESPNOW_WIFI_IF;
    broadcast_peer.encrypt = false;
    estd::copy_n(broadcast_mac, ESP_NOW_ETH_ALEN, broadcast_peer.peer_addr);
    ESP_ERROR_CHECK(esp_now_add_peer(&broadcast_peer));

    ESP_LOGI(TAG, "startup: vendor=%s board=%s",
        board_traits::vendor,
        board_traits::name);

    ESP_LOGI(TAG, "startup: sizeof App=%u, App::EspNow::recv_info=%u, wifi_pkt_rx_ctrl_t=%u",
        sizeof(App), sizeof(App::EspNow::recv_info),
        sizeof(wifi_pkt_rx_ctrl_t));

    estd::layer1::ostringstream<64> str;
    // DEBT: Doesn't have a data() method yet and also have to explicitly
    // include basic_string_view
    //const auto& s = str.rdbuf()->view();
    const auto& s = str.rdbuf()->str();
    
    for(;;)
    {
        static int counter = 0;

        ESP_LOGI(TAG, "counting: %d", ++counter);

        str.rdbuf()->clear();
        str << "Hello: " << counter;

        // DEBT: Observed one time where initial message was sent twice.  Unsure
        // why that is, debt until we discover why
        esp_err_t ret = esp_now_send(broadcast_mac, (const uint8_t*)s.data(), s.length());

        if(ret != ESP_OK)
        {
            ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
            ESP_LOGW(TAG, "Couldn't send");
        }

        estd::this_thread::sleep_for(3s);
    }
}


#include <esp-helper.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include <estd/chrono.h>
#include <estd/optional.h>
#include <estd/thread.h>

#include <embr/platform/esp-idf/board.h>

#include <embr/platform/esp-idf/service/diagnostic.h>
#include <embr/platform/esp-idf/service/twai.hpp>

#include <embr/exp/platform/esp-idf/transport/esp-now.h>
#include <embr/exp/platform/esp-idf/transport/twai.h>
#include <embr/exp/platform/esp-idf/transport/i2c.h>
#include <embr/exp/platform/lwip/transport/udp_pcb.h>

using Diagnostic = embr::esp_idf::service::v1::Diagnostic;
using board_traits = embr::esp_idf::board_traits;
namespace exp = embr::experimental::v1;

#include "app.h"


void App::on_notify(TWAI::event::alert alert)
{
    ESP_LOGI(TAG, "on_notify: TWAI alert=%" PRIx32, alert.alerts);
}


namespace app_domain {

App app;

typedef estd::integral_constant<App*, &app> app_singleton;
typedef embr::layer0::subject<Diagnostic, app_singleton> filter_observer;

App::TWAI::runtime<filter_observer> twai;

}


//template <class Transport, class Traits = exp::transport_traits<Transport>, bool = false>
//void test(Transport&);

template <class Transport, class Traits = exp::transport_traits<Transport>,
    estd::enable_if_t<Traits::nonexist == exp::SUPPORT_REQUIRED, bool> = true>
void test(Transport&)
{
    static constexpr const char* TAG = "test: n/a";

    ESP_LOGI(TAG, "GOT HERE 0");
}

template <class Transport, class Traits = exp::transport_traits<Transport>,
    estd::enable_if_t<Traits::polled == exp::SUPPORT_REQUIRED, bool> = true>
void test(Transport&)
{
    static constexpr const char* TAG = "test: polled";

    ESP_LOGI(TAG, "GOT HERE 1");
}

template <class Transport, class Traits = exp::transport_traits<Transport>,
    estd::enable_if_t<
        Traits::timeout == exp::SUPPORT_REQUIRED ||
        Traits::timeout == exp::SUPPORT_OPTIONAL, bool> = true>
void test(Transport& t)
{
    using mode = typename Traits::mode<exp::TRANSPORT_TRAIT_TIMEOUT>;
    using frame = typename Traits::frame_type;

    frame f;

    static constexpr const char* TAG = "test: timeout";

    ESP_LOGI(TAG, "GOT HERE 2");

    auto r = mode{}.write(&f, estd::chrono::seconds(1));

    switch(r)
    {
        case Traits::template error_type<exp::TRANSPORT_RET_OK>::value:
            break;
        
        default:
            break;
    }
}

exp::TwaiTransport transport;


extern "C" void app_main()
{
    const char* TAG = "app_main";

    ESP_LOGI(TAG, "Board: %s %s", board_traits::vendor, board_traits::name);

    app_domain::twai.start();

    test(transport);

    for(;;)
    {
        static int counter = 0;

        app_domain::twai.poll(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "counting: %d", ++counter);
    }
}


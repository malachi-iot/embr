#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include <led_strip.h>

#include <estd/chrono.h>
#include <estd/optional.h>
#include <estd/thread.h>

#include <embr/platform/esp-idf/board.h>

#include <embr/platform/esp-idf/service/diagnostic.h>
#include <embr/platform/esp-idf/service/flash.hpp>
#include <embr/platform/esp-idf/service/wifi.hpp>

#include <embr/platform/esp-idf/gpio.h>


namespace service = embr::esp_idf::service::v1;
using Diagnostic = service::Diagnostic;

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
using tier2 = embr::layer0::subject<Diagnostic, app_singleton>;

using wifi_type = service::WiFi::static_type<tier2>;

using tier1 = tier2::append<wifi_type>;

}


using namespace estd::chrono_literals;

using board_traits = embr::esp_idf::board_traits;

led_strip_handle_t configure_led(void);

#if FEATURE_EMBR_BOARD_STATUS_LED
constexpr embr::esp_idf::gpio status_led((gpio_num_t)board_traits::gpio::status_led);

void init_gpio()
{
    status_led.set_direction(GPIO_MODE_OUTPUT);
}
#elif defined(CONFIG_BOARD_ESP32C3_DEVKITM_1)

led_strip_handle_t led_strip;

void init_gpio()
{
    led_strip = configure_led();
}
#else
void init_gpio() {}
#endif


extern "C" void app_main()
{
    const char* TAG = "app_main";

    ESP_LOGI(TAG, "Board=%s %s", board_traits::vendor, board_traits::name);

    init_gpio();

    service::Flash::runtime<app_domain::tier1>{}.start();
    service::EventLoop::runtime<app_domain::tier1>{}.start();
    service::NetIf::runtime<app_domain::tier1>{}.start();

    wifi_config_t wifi_config = {};

    strcpy((char*)wifi_config.sta.ssid, CONFIG_WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, CONFIG_WIFI_PASSWORD);

    // err=0x1101 means NVS fail
    app_domain::wifi_type::value->start(WIFI_MODE_STA, &wifi_config);

    for(;;)
    {
        static int counter = 0;

        if(++counter % 5 == 0)  ESP_LOGI(TAG, "counting: %d", counter);

#if FEATURE_EMBR_BOARD_STATUS_LED
        status_led.level(counter % 2 == 0);
#elif defined(CONFIG_BOARD_ESP32C3_DEVKITM_1)
        led_strip_set_pixel(led_strip, 0, 
            (counter % 2 == 0) ? 20 : 0,
            0, 0);
        led_strip_refresh(led_strip);
#endif

        estd::this_thread::sleep_for(1s);
    }
}


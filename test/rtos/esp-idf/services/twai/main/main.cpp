#include <esp-helper.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include <estd/chrono.h>
#include <estd/optional.h>
#include <estd/thread.h>

#include <embr/platform/esp-idf/service/diagnostic.h>
#include <embr/platform/esp-idf/service/twai.hpp>


using Diagnostic = embr::esp_idf::service::v1::Diagnostic;

#include "app.h"


void App::on_notify(TWAI::event::alert alert)
{
    ESP_LOGI(TAG, "on_notify: TWAI alert=%" PRIx32, alert.alerts);
}


namespace app_domain {

App app;

typedef estd::integral_constant<App*, &app> app_singleton;
typedef embr::layer0::subject<Diagnostic, app_singleton> filter_observer;

embr::esp_idf::service::v1::TWAI::runtime<filter_observer> twai;

}




static void init_twai()
{
    static twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CONFIG_GPIO_TWAI_TX,
        (gpio_num_t)CONFIG_GPIO_TWAI_RX,
        TWAI_MODE_NORMAL);

    static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_125KBITS();

    app_domain::twai.start(&g_config, &t_config, &f_config);

    // We want all alerts since this is a diagnostic app
    ESP_ERROR_CHECK(twai_reconfigure_alerts(0xFFFF, nullptr));
}



extern "C" void app_main()
{
    const char* TAG = "app_main";

    init_twai();

    uint32_t prev_alerts = 0;

    esp_task_wdt_config_t wdt = {};

    wdt.timeout_ms = 100;

    esp_task_wdt_init(&wdt);

    ESP_ERROR_CHECK(esp_task_wdt_add(nullptr));
    ESP_ERROR_CHECK(esp_task_wdt_status(nullptr));

    for(;;)
    {
        static int counter = 0;

        //estd::this_thread::sleep_for(estd::chrono::seconds(1));
        estd::this_thread::sleep_for(estd::chrono::milliseconds(10));
        //estd::this_thread::yield();
        esp_task_wdt_reset();

        uint32_t alerts = 0;

        twai_read_alerts(&alerts, 0);

        if(alerts != prev_alerts)
        {
            ESP_LOGD(TAG, "alerts: %" PRIx32, alerts);
            prev_alerts = alerts;
            app_domain::twai.broadcast(alerts);
        }

        //ESP_LOGI(TAG, "counting: %d", ++counter);
    }
}


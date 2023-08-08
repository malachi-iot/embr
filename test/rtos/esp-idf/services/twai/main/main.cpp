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

void App::on_notify(TWAI::event::autorx rx)
{
    twai_message_t& message = *rx.message;
    
    ESP_LOGI(TAG, "on_notify: TWAI rx: id=%" PRIx32 " dlc=%u",
        message.identifier, message.data_length_code);
    ESP_LOG_BUFFER_HEX(TAG, message.data, message.data_length_code);
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
    app_domain::twai.autorx(true);
}



extern "C" void app_main()
{
    const char* TAG = "app_main";

    init_twai();

    for(;;)
    {
        static int counter = 0;

        app_domain::twai.poll(pdMS_TO_TICKS(5000));

        ESP_LOGI(TAG, "counting: %d", ++counter);
    }
}


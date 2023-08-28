#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include <estd/chrono.h>
#include <estd/optional.h>
#include <estd/thread.h>

#include <embr/platform/esp-idf/board.h>
#include <embr/platform/esp-idf/gpio.h>

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
    const twai_message_t& message = rx.message;
    
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

#if defined(CONFIG_BOARD_ESP32S3_UM_FEATHERS3)
#if CONFIG_GPIO_TWAI_RX == 4
#error "FeatherS3 can't operate TWAI RX on default pin 4"
#endif
#endif

#if !CONFIG_TWAI_TIMING
#error Please configure TWAI in Embr Helper area
#endif


using board_traits = embr::esp_idf::board_traits;


#if FEATURE_EMBR_BOARD_STATUS_LED
constexpr embr::esp_idf::gpio status_led(
    (gpio_num_t)board_traits::gpio::status_led);

void init_gpio()
{
    status_led.set_direction(GPIO_MODE_OUTPUT);
}
#else
void init_gpio() {}
#endif

#if defined(CONFIG_BOARD_ESP32_UNSPECIFIED)
#warning "Board not specified, but probably you'd prefer it to be"
#endif



extern "C" void app_main()
{
    const char* TAG = "app_main";

    ESP_LOGI(TAG, "Board: %s %s", board_traits::vendor, board_traits::name);

    app_domain::twai.start();
    app_domain::twai.autorx(true);

    init_gpio();

    for(;;)
    {
        static int counter = 0;

        app_domain::twai.poll(pdMS_TO_TICKS(5000));

        ESP_LOGI(TAG, "counting: %d", ++counter);

#if FEATURE_EMBR_BOARD_STATUS_LED
        status_led.level(counter % 2 == 0);
#endif
    }
}


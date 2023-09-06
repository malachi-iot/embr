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

#include <embr/exp/platform/esp-idf/transport/twai.h>

using Diagnostic = embr::esp_idf::service::v1::Diagnostic;
using board_traits = embr::esp_idf::board_traits;

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



extern "C" void app_main()
{
    const char* TAG = "app_main";

    ESP_LOGI(TAG, "Board: %s %s", board_traits::vendor, board_traits::name);

    app_domain::twai.start();

    for(;;)
    {
        static int counter = 0;

        app_domain::twai.poll(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "counting: %d", ++counter);
    }
}


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

}


namespace app_domain {

App app;

typedef estd::integral_constant<App*, &app> app_singleton;
typedef embr::layer0::subject<Diagnostic, app_singleton> filter_observer;

embr::esp_idf::service::v1::TWAI::runtime<filter_observer> twai;

}




static void init_twai()
{
}



extern "C" void app_main()
{
    const char* TAG = "app_main";

    init_twai();

    for(;;)
    {
        static int counter = 0;

        estd::this_thread::sleep_for(estd::chrono::seconds(1));

        ESP_LOGI(TAG, "counting: %d", ++counter);
    }
}


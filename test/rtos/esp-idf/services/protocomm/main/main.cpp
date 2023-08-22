#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include <estd/chrono.h>
#include <estd/optional.h>
#include <estd/thread.h>

#include <embr/platform/esp-idf/service/diagnostic.h>
#include <embr/platform/esp-idf/service/event.hpp>
#include <embr/platform/esp-idf/service/flash.hpp>
#include <embr/platform/esp-idf/service/protocomm.hpp>


namespace service = embr::esp_idf::service::v1;
using Diagnostic = service::Diagnostic;

#include "app.h"


namespace app_domain {

App app;

typedef estd::integral_constant<App*, &app> app_singleton;
using tier2 = embr::layer0::subject<Diagnostic, app_singleton>;

using tier1 = tier2;

}


using namespace estd::chrono_literals;


extern "C" void app_main()
{
    const char* TAG = "app_main";

    service::Flash::runtime<app_domain::tier1>{}.start();
    service::EventLoop::runtime<app_domain::tier1>{}.start();

    for(;;)
    {
        static int counter = 0;

        ESP_LOGI(TAG, "counting: %d", ++counter);

        estd::this_thread::sleep_for(5s);
    }
}


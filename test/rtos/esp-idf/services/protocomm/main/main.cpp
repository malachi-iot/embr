#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include <protocomm_security0.h>
#include <protocomm_security1.h>


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


static constexpr const char ep1[] = "test2";
static constexpr const char* const ep2 = "test2";

void App::do_notify(service::Protocomm::event::tag<int> e)
{

}


void App::do_notify(service::Protocomm::event::request_named<ep3> e)
{

}


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

    service::Protocomm::runtime<app_domain::tier1> protocomm;

    protocomm.start();
    
    /* Create security1 params object from pop_string. It must be valid throughout the scope of protocomm endpoint. This need not be static, i.e., could be dynamically allocated and freed at the time of endpoint removal. */
    const char* pop_string = "password";
    const static protocomm_security1_params_t sec1_params = {
        .data = (const uint8_t *) strdup(pop_string),
        .len = (uint16_t)strlen(pop_string)
    };

    protocomm.set_security("security_endpoint", &protocomm_security1, &sec1_params);

    protocomm.add_endpoint<int>("test");
    protocomm.add_endpoint<App::ep3>();

    for(;;)
    {
        static int counter = 0;

        ESP_LOGI(TAG, "counting: %d", ++counter);

        estd::this_thread::sleep_for(5s);
    }
}


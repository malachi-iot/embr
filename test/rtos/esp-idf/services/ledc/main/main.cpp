#include "esp_log.h"
#include "esp_system.h"

#include <estd/chrono.h>
#include <estd/optional.h>
#include <estd/thread.h>

#include <embr/platform/esp-idf/board.h>

#include <embr/platform/esp-idf/service/flash.hpp>
#include <embr/platform/esp-idf/service/ledc.hpp>

namespace service = embr::esp_idf::service::v1;

#include "app.h"
#include "services.h"

namespace app_domain { App app; }

// Adapted from
// https://github.com/espressif/esp-idf/blob/82cceabc6e6a0a2d8c40e2249bc59917cc5e577a/examples/peripherals/ledc/ledc_basic/main/ledc_basic_example_main.c
// https://github.com/espressif/esp-idf/blob/v5.1.1/examples/peripherals/ledc/ledc_fade/main/ledc_fade_example_main.c

using namespace estd::chrono_literals;

void ledc_init()
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_fade_func_install(0);
}


extern "C" void app_main()
{
    const char* TAG = "app_main";

    service::Flash::runtime<app_domain::tier1>{}.start();

    app_domain::ledc_type ledc;

    ledc_init();
    ledc_init_iram(ledc);

    ledc.start();

    for(;;)
    {
        static int counter = 0;

        ESP_LOGI(TAG, "counting: %d", ++counter);

        estd::this_thread::sleep_for(5s);
    }
}


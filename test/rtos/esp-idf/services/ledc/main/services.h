#pragma once

#include "app.h"

#include <embr/platform/esp-idf/service/diagnostic.h>

namespace service = embr::esp_idf::service::v1;

namespace app_domain {

using Diagnostic = service::Diagnostic;

extern App app;

typedef estd::integral_constant<App*, &app> app_singleton;
using tier2 = embr::layer0::subject<Diagnostic, app_singleton>;

using tier1 = tier2;

using ledc_type = service::LEDControl::runtime<app_domain::tier1>;

}


void ledc_init_iram(app_domain::ledc_type&);

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          CONFIG_LEDC_OUTPUT_IO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz


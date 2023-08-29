#include <embr/platform/esp-idf/service/ledc.hpp>

#include "app.h"
#include "services.h"


void App::on_notify(service::LEDControl::event::callback e)
{
    
}



void ledc_init_iram(app_domain::ledc_type& ledc)
{
    // Prepare and then apply the LEDC PWM channel configuration
    static constexpr ledc_channel_config_t ledc_channel = {
        .gpio_num       = LEDC_OUTPUT_IO,
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0,
        .flags {
            .output_invert = 0
        }
    };

    ledc.config(ledc_channel);
}
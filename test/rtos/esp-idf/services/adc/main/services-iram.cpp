#include <embr/platform/esp-idf/service/adc.hpp>

#include "app.h"


// callback won't be inline, but its cascade out to notify can be
inline void App::on_notify(ADC::event::converted e)
{
    q2.send_from_isr({0});
}

// Has to reside in IRAM module because adc.start generates the
// callback which itself must be in IRAM
void start(const adc_continuous_handle_cfg_t* frame_config,
    const adc_continuous_config_t* config)
{
    app_domain::adc.start(frame_config, config);
}

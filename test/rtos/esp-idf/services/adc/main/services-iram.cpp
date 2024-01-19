#include <embr/platform/esp-idf/service/adc.hpp>

#include "app.h"

volatile int isr_counter = 0;
volatile int isr_overrun = 0;

// callback won't be inline, but its cascade out to notify can be
inline void App::on_notify(ADC::event::converted e)
{
    // Be quick about it!
    // Using begin() and end() ... cool
    for(ADC::event::converted::reference p : e)
    {
    }

    // DEBT: Do e.must_yield for more performance
    BaseType_t success = q.send_from_isr({e.begin(), e.end()});
    if(success == pdFALSE)  isr_overrun = isr_overrun + 1;

    isr_counter = isr_counter + 1;
}

// Has to reside in IRAM module because adc.start generates the
// callback which itself must be in IRAM [2]
void App::start(const adc_continuous_handle_cfg_t* frame_config,
    const adc_continuous_config_t* config)
{
    app_domain::adc.start(frame_config, config);
}

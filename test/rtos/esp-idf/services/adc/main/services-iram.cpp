#include <embr/platform/esp-idf/service/adc.hpp>

#include "app.h"

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
#define ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE1
#define ADC_GET_CHANNEL(p_data)     ((p_data)->type1.channel)
#define ADC_GET_DATA(p_data)        ((p_data)->type1.data)
#else
#define ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE2
#define ADC_GET_CHANNEL(p_data)     ((p_data)->type2.channel)
#define ADC_GET_DATA(p_data)        ((p_data)->type2.data)
#endif

volatile int isr_counter = 0;

// callback won't be inline, but its cascade out to notify can be
inline void App::on_notify(ADC::event::converted e)
{
    // Be quick about it!
    for (int i = 0; i < e.edata->size; i += SOC_ADC_DIGI_RESULT_BYTES)
    {
        auto p = (const adc_digi_output_data_t *)&e.edata->conv_frame_buffer[i];

        uint32_t chan_num = ADC_GET_CHANNEL(p);
        uint32_t data = ADC_GET_DATA(p);
    }

    q.send_from_isr({e.begin(), e.end()});
    isr_counter = isr_counter + 1;
}

// Has to reside in IRAM module because adc.start generates the
// callback which itself must be in IRAM [2]
void App::start(const adc_continuous_handle_cfg_t* frame_config,
    const adc_continuous_config_t* config)
{
    app_domain::adc.start(frame_config, config);
}

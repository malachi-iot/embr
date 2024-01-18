#include "app.h"

// https://docs.espressif.com/projects/esp-idf/en/v5.1.2/esp32/api-reference/peripherals/adc_continuous.html

#define EXAMPLE_READ_LEN   256

#if CONFIG_IDF_TARGET_ESP32
#define ADC_CONV_MODE       ADC_CONV_SINGLE_UNIT_1  //ESP32 only supports ADC1 DMA mode
#define ADC_OUTPUT_TYPE     ADC_DIGI_OUTPUT_FORMAT_TYPE1
#elif CONFIG_IDF_TARGET_ESP32S2
#define ADC_CONV_MODE       ADC_CONV_BOTH_UNIT
#define ADC_OUTPUT_TYPE     ADC_DIGI_OUTPUT_FORMAT_TYPE2
#elif CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32H2 || CONFIG_IDF_TARGET_ESP32C2
#define ADC_CONV_MODE       ADC_CONV_ALTER_UNIT     //ESP32C3 only supports alter mode
#define ADC_OUTPUT_TYPE     ADC_DIGI_OUTPUT_FORMAT_TYPE2
#elif CONFIG_IDF_TARGET_ESP32S3
#define ADC_CONV_MODE       ADC_CONV_BOTH_UNIT
#define ADC_OUTPUT_TYPE     ADC_DIGI_OUTPUT_FORMAT_TYPE2
#endif

#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32H2 || CONFIG_IDF_TARGET_ESP32C2
//static adc_channel_t channel[3] = {ADC_CHANNEL_2, ADC_CHANNEL_3, (adc_channel_t)(ADC_CHANNEL_0 | 1 << 3)};
static adc_channel_t channel[] = {ADC_CHANNEL_1};
#endif
#if CONFIG_IDF_TARGET_ESP32S2
static adc_channel_t channel[3] = {ADC_CHANNEL_2, ADC_CHANNEL_3, (adc_channel_t)(ADC_CHANNEL_0 | 1 << 3)};
#endif
#if CONFIG_IDF_TARGET_ESP32
static adc_channel_t channel[1] = {ADC_CHANNEL_7};
#endif

#define GET_UNIT(x)        ((x>>3) & 0x1)

void App::start()
{
    adc_continuous_handle_cfg_t frame_config =
    {
        // DEBT: Document here what diff is between frame size and store buf
        // size
        .max_store_buf_size = 1024,
        // TODO: Increase this number for regular ESP32 since it reads so fast,
        // we get interrupted a LOT
        .conv_frame_size = EXAMPLE_READ_LEN,
    };

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX];

#if defined(CONFIG_MIOT_ADC_BATTERY)
    adc_unit_t unit_id;
    adc_channel_t _channel;
    adc_digi_pattern_config_t& adcp = adc_pattern[0];
    ESP_ERROR_CHECK(adc_continuous_io_to_channel(CONFIG_MIOT_ADC_BATTERY, 
        &unit_id, &_channel));
    ESP_LOGD(TAG, "GPIO of %u maps to ADC unit %u channel %u",
        CONFIG_MIOT_ADC_BATTERY, unit_id, _channel);

    adcp.unit = unit_id;
    adcp.channel = _channel;

    constexpr int channel_count = 1;
#else
    // Go into example mode - for this really to be useful we should sample
    // a whole bunch of ADCs
    constexpr int channel_count = sizeof(channel) / sizeof(adc_channel_t);
#endif

    adc_continuous_config_t config =
    {
        .pattern_num = channel_count,
        .adc_pattern = adc_pattern,
        // Relatively relaxed Hz, our battery monitoring needs are not that intense
        // ESP32C3 = 611
        // ESP32 = 20000
        .sample_freq_hz = SOC_ADC_SAMPLE_FREQ_THRES_LOW,
        .conv_mode = ADC_CONV_MODE,
        // TODO: See adc_digi_output_data_t type
        .format = ADC_OUTPUT_TYPE,
    };

#if defined(CONFIG_MIOT_ADC_BATTERY)
    {
        adcp.atten = ADC_ATTEN_DB_11;
        adcp.bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;
    }
#else
    for(int i = 0; i < channel_count; ++i)
    {
        uint8_t unit = GET_UNIT(channel[i]);
        uint8_t ch = channel[i] & 0x7;
        adc_digi_pattern_config_t& adcp = adc_pattern[i];

        adcp.channel = ch;
        adcp.unit = unit;

        //adcp.atten = ADC_ATTEN_DB_0;
        adcp.atten = ADC_ATTEN_DB_11;
        adcp.bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;
    }
#endif

    start(&frame_config, &config);
}

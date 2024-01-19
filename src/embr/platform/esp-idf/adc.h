#pragma once

#include <hal/adc_types.h>

namespace embr { namespace esp_idf { namespace adc { inline namespace v1 {

// All because of pesky ESP32S2, we need both TYPE1 and TYPE2 onhand -
// it uniquely can do either

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
#define SOC_ADC_DIGI_HAS_TYPE1 1
#endif

// DEBT: Some descrepency whether S2 prefers TYPE1 or TYPE2
#if SOC_ADC_DIGI_HAS_TYPE1
template <adc_digi_output_format_t = ADC_DIGI_OUTPUT_FORMAT_TYPE1>
#else
template <adc_digi_output_format_t = ADC_DIGI_OUTPUT_FORMAT_TYPE2>
#endif
struct digi_output_data;

#if SOC_ADC_DIGI_HAS_TYPE1
template <>
struct digi_output_data<ADC_DIGI_OUTPUT_FORMAT_TYPE1> : adc_digi_output_data_t
{
    static constexpr const adc_digi_output_format_t format = ADC_DIGI_OUTPUT_FORMAT_TYPE1;

    using value_type = uint16_t;

    constexpr value_type data() const { return type1.data; }
    constexpr value_type channel() const { return type1.channel; }
};
#endif


template <>
struct digi_output_data<ADC_DIGI_OUTPUT_FORMAT_TYPE2> : adc_digi_output_data_t
{
    static constexpr const adc_digi_output_format_t format = ADC_DIGI_OUTPUT_FORMAT_TYPE2;

    // DEBT: Would use SOC_ADC_DIGI_RESULT_BYTES but C2 seems to be missing that one
#if SOC_ADC_DIGI_HAS_TYPE1
    using value_type = uint16_t;
#else
    using value_type = uint32_t;
#endif

    constexpr value_type data() const { return type2.data; }
    constexpr value_type channel() const { return type2.channel; }
#if SOC_ADC_DIGI_CONTROLLER_NUM > 1
    constexpr adc_unit_t unit() const { return (adc_unit_t)type2.unit; }
#endif
};


}}}}
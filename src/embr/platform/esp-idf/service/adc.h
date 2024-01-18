#pragma once

#include <esp_adc/adc_continuous.h>
#include <embr/service.h>

namespace embr { namespace esp_idf {

namespace service { inline namespace v1 {

namespace experimental {

template <class TService>
bool IRAM_ATTR adc_conv_done_cb(
    adc_continuous_handle_t handle,
    const adc_continuous_evt_data_t *edata, void *user_data);

bool IRAM_ATTR adc_conv_done_cb2(
    adc_continuous_handle_t handle,
    const adc_continuous_evt_data_t *edata, void *user_data);

bool IRAM_ATTR adc_conv_done_cb3(
    adc_continuous_handle_t handle,
    const adc_continuous_evt_data_t *edata, void *user_data);

bool adc_conv_done_cb4(
    adc_continuous_handle_t handle,
    const adc_continuous_evt_data_t *edata, void *user_data);

}

// Specifically continuous mode
struct ADC : embr::Service
{
    // DEBT: This is magic convention that EMBR_PROPERTY_RUNTIME_BEGIN needs
    typedef ADC this_type;

    typedef embr::Service base_type;

    static constexpr const char* TAG = "embr::ADC";
    static constexpr const char* name() { return TAG; }

    adc_continuous_handle_t handle;

    struct event
    {
        // NOTE: Be advised this is fired from ISR context
        struct converted
        {
            const adc_continuous_handle_t handle;   // NOTE: This is available via runtime
            const adc_continuous_evt_data_t* const edata;
            BaseType_t* const must_yield;
        };
    };

    EMBR_PROPERTY_RUNTIME_BEGIN(embr::Service)

        static bool s_conv_done_cb(
            adc_continuous_handle_t handle,
            const adc_continuous_evt_data_t *edata, void *user_data);
        
        state_result on_start(
            const adc_continuous_handle_cfg_t*,
            const adc_continuous_config_t*);

    EMBR_PROPERTY_RUNTIME_END
};

}}

}}
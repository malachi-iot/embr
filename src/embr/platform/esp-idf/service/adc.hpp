#pragma once

#include "adc.h"

#include <esp_log.h>
#include <esp_check.h>

// Due to https://github.com/espressif/esp-idf/issues/4542
// we'll need to use IRAM methods from a special IRAM module
// via a linker script (see test/service/adc)

namespace embr { namespace esp_idf {

namespace service { inline namespace v1 {

// NOTE: Watch out for https://github.com/espressif/esp-idf/issues/4542 -
// you'll have to do a linker fragment file
template <class TSubject, class TImpl>
bool IRAM_ATTR ADC::runtime<TSubject, TImpl>::conv_done_cb(
    adc_continuous_handle_t handle,
    const adc_continuous_evt_data_t* edata, void* user_data)
{
    BaseType_t mustYield = pdFALSE;

    ((runtime*) user_data)->notify(event::converted{
        handle,
        edata, &mustYield});

    return (mustYield == pdTRUE);
}

template <class TSubject, class TImpl>
auto ADC::runtime<TSubject, TImpl>::on_start(
    const adc_continuous_handle_cfg_t* frame_config,
    const adc_continuous_config_t* config) ->
    state_result
{
    [[maybe_unused]] esp_err_t ret;
    adc_continuous_handle_t& handle = base_type::handle;

    // TODO: Set up helper macro to include usings for
    // - configured
    // - configuring
    // - progress
    // - state
    base_type::configuring(frame_config);
    ESP_GOTO_ON_ERROR(
        adc_continuous_new_handle(frame_config, &handle),
        err, TAG, "couldn't configure initial frame");
    base_type::configured(frame_config);
    base_type::configuring(config);
    ESP_GOTO_ON_ERROR(
        adc_continuous_config(handle, config),
        err, TAG, "couldn't configure ADC");

    base_type::configured(config);

    {
        adc_continuous_evt_cbs_t cbs =
        {
            // NOTE: Watch out for templated IRAM glitch
            .on_conv_done = conv_done_cb,
            // DEBT: Do overflow event too.  May not ever care because
            // this event service you'll likely want pure DMA mode.  See
            // PGESP-41 README section 3
            .on_pool_ovf = nullptr,
        };
        ESP_GOTO_ON_ERROR(adc_continuous_register_event_callbacks(handle, &cbs, this),
            err, TAG, "couldn't configure callback");
    }

    base_type::state(Configured);
    
    ESP_GOTO_ON_ERROR(adc_continuous_start(handle),
        err, TAG, "couldn't start ADC driver after config");

    return state_result::started();
err:

    return state_result{Error, ErrConfig};
}

}}
}}
#pragma once

#include "timer.h"


template <class TSubject, class TImpl>
bool TimerService::runtime<TSubject, TImpl>::on_alarm_cb(
    gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    ((runtime*)user_ctx)->on_alarm_cb(edata);
    return false;
}


template <class TSubject, class TImpl>
TimerService::state_result TimerService::runtime<TSubject, TImpl>::on_start(
    const gptimer_config_t* config,
    const gptimer_alarm_config_t* alarm_config)
{
    constexpr state_result error_result{Error, ErrConfig};

    ESP_LOGI(TAG, "on_start: runtime=%p", this);

    embr::esp_idf::Timer t = impl().t;

    esp_err_t err;

    base_type::configuring(config);

    err = t.init(config);

    if(err != ESP_OK) return error_result;

    base_type::configured(config);

    if(alarm_config)
    {
        base_type::configuring(alarm_config);

        err = t.set_alarm_action(alarm_config);

        if(err != ESP_OK) return error_result;

        base_type::configured(alarm_config);
    }

    gptimer_event_callbacks_t cbs =
    {
        .on_alarm = runtime<TSubject, TImpl>::on_alarm_cb, // register user callback
    };
    
    err = t.register_event_callbacks(&cbs, this);

    err = t.enable();
    err = t.start();

    return err == ESP_OK ? state_result::started() : error_result;
}

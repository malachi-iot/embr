#pragma once

#include <embr/platform/esp-idf/gptimer.h>


#include <embr/service.h>

struct TimerService : embr::Service
{
    typedef TimerService this_type;

    static constexpr const char* TAG = "TimerService";
    
    constexpr static const char* name() { return TAG; }

    embr::esp_idf::v5::Timer t;

    enum Dummy { DUMMY };

    EMBR_PROPERTIES_SPARSE_BEGIN

        // DEBT: Revise API with 'auto -2' flavor
        // DEBT: Formalize -2
        EMBR_PROPERTY_SPARSE_ID(timer, uint32_t, DUMMY, "timer in ms")
        //EMBR_PROPERTY_ID(timer, uint32_t, "timer in ms");

    EMBR_PROPERTIES_SPARSE_END

    bool stop()
    {
        t.stop();
        return true;
    }

    template <class TSubject, class TImpl = this_type>
    struct runtime : embr::Service::runtime<TSubject, TImpl>
    {
        typedef embr::Service::runtime<TSubject, TImpl> base_type;
        
        // DEBT: EMBR_PROPERTY requires this, make this part automatic
        // (likely in macro do base_type::impl_type)
        using typename base_type::impl_type;

        using base_type::impl;

        ESTD_CPP_FORWARDING_CTOR(runtime)

        //EMBR_PROPERTY(timer)
        void timer(uint32_t v)
        {
            base_type::template fire_changed<id::timer>(v);
        }

        void thunk(const gptimer_alarm_event_data_t* edata)
        {
            //uint64_t count;
            //impl().t.raw_count(&count);
            timer(edata->count_value / 1000);
        }

        static bool on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);

        state_result on_start(
            const gptimer_config_t* config,
            const gptimer_alarm_config_t* alarm_config = nullptr);
    };
};


template <class TSubject, class TImpl>
bool TimerService::runtime<TSubject, TImpl>::on_alarm_cb(
    gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    ((runtime*)user_ctx)->thunk(edata);
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

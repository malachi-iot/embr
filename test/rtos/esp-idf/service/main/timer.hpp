#pragma once

#include <embr/platform/esp-idf/gptimer.h>


#include <embr/service.h>

struct TimerService : embr::Service
{
    typedef TimerService this_type;

    static constexpr const char* TAG = "TimerService";
    
    embr::esp_idf::v5::Timer t;

    enum Dummy { DUMMY };

    EMBR_PROPERTIES_SPARSE_BEGIN

        // DEBT: Revise API with 'auto -2' flavor
        // DEBT: Formalize -2
        EMBR_PROPERTY_SPARSE_ID(timer, uint32_t, DUMMY, "timer in ms")
        //EMBR_PROPERTY_ID(timer, uint32_t, "timer in ms");

    EMBR_PROPERTIES_SPARSE_END

    state_result start(
        const gptimer_config_t* config,
        const gptimer_alarm_config_t* alarm_config = nullptr)
    {
        esp_err_t err;

        err = t.init(config);

        if(err != ESP_OK)
            return state_result{Error, ErrConfig};

        if(alarm_config)
        {
            err = t.set_alarm_action(alarm_config);

            if(err != ESP_OK)
                return state_result{Error, ErrConfig};
        }
        return state_result{Started, Running};
    }


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
    };

    //template <class TSubject, class TImpl>
    //bool on_start(context<TSubject, TImpl>& context)
    // DEBT: Fix up naming - this is a special phase of start, called
    // after this above one, which can receive the runtime in question.
    // Consider combining the two
    template <class TSubject, class TImpl>
    bool on_start(runtime<TSubject, TImpl>& r)
    {
        ESP_LOGI(TAG, "on_start: runtime=%p", &r);

        //embr::esp_idf::Timer t = impl().t;

        gptimer_event_callbacks_t cbs =
        {
            .on_alarm = runtime<TSubject, TImpl>::on_alarm_cb, // register user callback
        };
        
        t.register_event_callbacks(&cbs, &r);

        t.enable();
        t.start();
        return true;
    }
};


template <class TSubject, class TImpl>
bool TimerService::runtime<TSubject, TImpl>::on_alarm_cb(
    gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    ((runtime*)user_ctx)->thunk(edata);
    return false;
}
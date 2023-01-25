#pragma once

#include <embr/platform/esp-idf/gptimer.h>


#include <embr/service.h>

struct TimerService : embr::Service
{
    typedef TimerService this_type;

    embr::esp_idf::v5::Timer t;

    enum Dummy { DUMMY };

    EMBR_PROPERTIES_SPARSE_BEGIN

        // DEBT: Revise API with 'auto -2' flavor
        // DEBT: Formalize -2
        EMBR_PROPERTY_SPARSE_ID(timer, uint32_t, DUMMY, "timer in ms")
        //EMBR_PROPERTY_ID(timer, uint32_t, "timer in ms");

    EMBR_PROPERTIES_SPARSE_END

    bool start(
        const gptimer_config_t* config,
        const gptimer_alarm_config_t* alarm_config = nullptr)
    {
        t.init(config);
        if(alarm_config)
        {
            t.set_alarm_action(alarm_config);
        }
        t.enable();
        t.start();
        return true;
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

        void _start()
        {
            gptimer_event_callbacks_t cbs =
            {
                .on_alarm = on_alarm_cb, // register user callback
            };
            impl().t.register_event_callbacks(&cbs, this);
        }
    };
};


template <class TSubject, class TImpl>
bool TimerService::runtime<TSubject, TImpl>::on_alarm_cb(
    gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    ((runtime*)user_ctx)->thunk(edata);
    return false;
}
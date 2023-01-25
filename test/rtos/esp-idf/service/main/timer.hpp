#pragma once

#include <embr/platform/esp-idf/gptimer.h>


#include <embr/service.h>

struct TimerService : embr::Service
{
    typedef TimerService this_type;

    embr::esp_idf::v5::Timer t;

    EMBR_PROPERTIES_SPARSE_BEGIN

        // DEBT: Revise API with 'auto -2' flavor
        // DEBT: Formalize -2
        EMBR_PROPERTY_SPARSE_ID(timer, uint32_t, -2, "timer in ms")
        //EMBR_PROPERTY_ID(timer, uint32_t, "timer in ms");

    EMBR_PROPERTIES_SPARSE_END

    bool start(const gptimer_config_t* config)
    {
        t.init(config);
        return true;
    }

    template <class TSubject, class TImpl = this_type>
    struct runtime : embr::Service::runtime<TSubject, TImpl>
    {
        typedef embr::Service::runtime<TSubject, TImpl> base_type;

        using base_type::impl;

        ESTD_CPP_FORWARDING_CTOR(runtime)

        EMBR_PROPERTY(timer)

        void thunk() {}

        static bool on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);

        void _start()
        {
            impl().t.register_event_callbacks(on_alarm_cb, this);
        }
    };
};


template <class TSubject, class TImpl>
bool TimerService::runtime<TSubject, TImpl>::on_alarm_cb(
    gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    ((runtime*)user_ctx)->thunk();
    return false;
}
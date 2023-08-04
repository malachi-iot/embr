#pragma once

#include <embr/platform/esp-idf/gptimer.h>

#include <embr/service.h>

namespace embr::esp_idf {

namespace service { inline namespace v1 {

struct GPTimer : embr::Service
{
    typedef GPTimer this_type;

    static constexpr const char* TAG = "Timer";
    constexpr static const char* name() { return TAG; }

    embr::esp_idf::v5::Timer t;
    void* user_data = nullptr;

    struct event
    {
        struct callback
        {
            const gptimer_handle_t timer;
            const gptimer_alarm_event_data_t& edata;
        };
    };

    // DEBT: Not used now - keeping around for use in a filter later
    EMBR_PROPERTIES_SPARSE_BEGIN

        // DEBT: Change this to a estd::chrono::duration of some variety
        // and consider hanging the whole service off that to determine
        // timer res
        EMBR_PROPERTY_ID_SPARSE(timer, uint32_t, "timer in ms")

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
        using base_type::state;

        ESTD_CPP_FORWARDING_CTOR(runtime)

        void timer(uint32_t v)
        {
            base_type::template fire_changed<id::timer>(v);
        }

        void on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata);

        static bool on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);

        state_result on_start(
            const gptimer_config_t* config,
            const gptimer_alarm_config_t* alarm_config = nullptr);
    };
};

}}

}

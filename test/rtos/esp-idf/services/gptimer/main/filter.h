#pragma once

#include <embr/platform/esp-idf/service/gptimer.h>

template <class Period>
struct TimerFilterService : embr::SparseService
{
    typedef TimerFilterService this_type;

    using TimerService = embr::esp_idf::service::v1::GPTimer;
    using period = Period;
    using duration = estd::chrono::duration<uint64_t, period>;

    struct event
    {
        struct callback
        {
            TimerService::event::callback& c;

            duration count_value() const
            {
                return { c.edata.count_value };
            }

            duration alarm_value() const
            {
                return { c.edata.alarm_value };
            }
        };
    };

    static constexpr const char* TAG = "TimerFilterService";
    constexpr static const char* name() { return TAG; }

    template <class TSubject, class TImpl = this_type>
    struct runtime : embr::Service::runtime<TSubject, TImpl>
    {
        typedef embr::Service::runtime<TSubject, TImpl> base_type;

        using base_type::notify;

        ESTD_CPP_FORWARDING_CTOR(runtime)

        void on_notify(TimerService::event::callback e)
        {
            ESP_DRAM_LOGI(TAG, "on_notify");
            notify(typename event::callback{e});
        }
    };
};

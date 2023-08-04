#pragma once

#include <embr/platform/esp-idf/service/gptimer.h>

struct TimerFilterService : embr::Service
{
    typedef TimerFilterService this_type;

    static constexpr const char* TAG = "TimerFilterService";
    
    constexpr static const char* name() { return TAG; }

    template <class TSubject, class TImpl = this_type>
    struct runtime : embr::Service::runtime<TSubject, TImpl>
    {
        typedef embr::Service::runtime<TSubject, TImpl> base_type;

        ESTD_CPP_FORWARDING_CTOR(runtime)

        void on_notify(TimerService::event::callback e)
        {
            ESP_DRAM_LOGI(TAG, "on_notify");
        }
    };
};

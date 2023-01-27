#pragma once

#include <embr/platform/esp-idf/gptimer.h>

#include <embr/service.h>

#include <esp_sleep.h>

#include "timer.h"

#define FEATURE_EMBR_ESP_SERVICE_SLEEP 1

struct SystemService : embr::Service
{
    typedef SystemService this_type;

    static constexpr const char* TAG = "esp-idf::SystemService";
    
    constexpr static const char* name() { return TAG; }

    struct event
    {
        struct wake
        {
            const esp_sleep_source_t cause;
        };
    };

    template <class TSubject, class TImpl = this_type>
    struct runtime : embr::Service::runtime<TSubject, TImpl>
    {
        typedef embr::Service::runtime<TSubject, TImpl> base_type;

        ESTD_CPP_FORWARDING_CTOR(runtime)

        bool wake();
        void sleep();

        void on_starting()
        {
            if(wake())
                base_type::state(Waking);
        }
    };
};

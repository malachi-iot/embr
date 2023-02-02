#pragma once

#include <estd/chrono.h>

#include "core.h"

#include <esp_sleep.h>

#define FEATURE_EMBR_ESP_SERVICE_SLEEP 1

namespace embr::esp_idf {

namespace service { inline namespace v1 {

// Pertains specifically to deep sleep management
struct PowerManager : embr::Service
{
    typedef PowerManager this_type;

    static constexpr const char* TAG = "esp-idf::PowerManager";
    
    constexpr static const char* name() { return TAG; }

    struct event
    {
        struct wake
        {
            const esp_sleep_source_t cause;
        };

        struct sleep_for
        {
            const estd::chrono::microseconds duration;
        };
    };

    template <class TSubject, class TImpl = this_type>
    struct runtime : embr::Service::runtime<TSubject, TImpl>
    {
        typedef embr::Service::runtime<TSubject, TImpl> base_type;

        ESTD_CPP_FORWARDING_CTOR(runtime)

        bool wake();
        void sleep();
        void sleep_for(estd::chrono::microseconds duration);

        void on_starting()
        {
            if(wake())
                base_type::state(Waking);
        }
    };
};

}}

}
#pragma once

#include "core.h"
#include "../ledc.h"

namespace embr::esp_idf {

namespace service { inline namespace v1 {

//template <unsigned channel_count>
struct LEDControl : embr::service::v1::Service
{
    using this_type = LEDControl;

    static constexpr const char* TAG = "LED Control";
    static constexpr const char* name() { return TAG; }

    // DEBT: Not even sure we really need to track all this, maybe a sparse
    // service will do?  We're mainly here to wrap up callbacks
    static constexpr const unsigned channel_count = 3;

    ledc ledc_[channel_count];

    struct event
    {
        // DEBT: Combine this with isr tag elsewhere
        // DEBT: Glance at timer scheduler ISR stuff maybe combine how it
        // handles woken as well

        struct callback
        {
            const ledc_cb_param_t& param;
            bool* woken;
        };
    };

    EMBR_SERVICE_RUNTIME_BEGIN(Service)

    void config(const ledc_timer_config_t&, unsigned intr_alloc = 0);
    void config(const ledc_channel_config_t&);

    IRAM_ATTR static bool callback(const ledc_cb_param_t*, void*);

    EMBR_SERVICE_RUNTIME_END
};

}}

}
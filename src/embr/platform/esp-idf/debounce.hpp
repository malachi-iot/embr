#pragma once

#include "../../detail/debounce.hpp"
#include "debounce.h"

#include "log.h"

ASSERT_EMBR_LOG_GROUP_MODE(2, EMBR_LOG_GROUP_MODE_ISR)

namespace embr { 

namespace scheduler { namespace esp_idf { namespace impl {

// NOTE: IRAM_ATTR doesn't make any ms-visible speed difference
template <int divider_>
bool Threshold<divider_>::process(value_type v, time_point now)
{
    time_point delta = now - v->last_wakeup_;

    detail::Debouncer& d = v->debouncer();
    const detail::Debouncer& d2 = d;    // DEBT: Workaround to get at const noise_or_signal
    bool level = v->pin().level();

// Same speed really either way in Debug mode
//#if DISABLED_WHILE_DIAGNOSING_SPEED
    detail::Debouncer::duration d_now(now);
    ESP_DRAM_LOGV(TAG, "process: state=%s:%s, level=%u, event_due=%llu, now=%llu, d_now=%llu, delta=%llu",
        to_string(d.state()), to_string(d.substate()), level,
        v->event_due(), now.count(), d_now.count(), delta.count());
//#endif

    bool state_changed = d.time_passed(delta, level);
    ESP_DRAM_LOGD(TAG, "process: state=%s:%s, changed=%u, strength=%u",
        to_string(d.state()), to_string(d.substate()), state_changed, d2.noise_or_signal());

    if(state_changed)
    {
        v->parent_->emit_state(*v);
    }
    else
    {
        // evaluate whether we need more time and if so, reschedule
        if(d.substate() != detail::Debouncer::Idle)
        {
            auto amt = d2.noise_or_signal() - d2.signal_threshold();

            // DEBT: Fudge factor
            amt += estd::chrono::milliseconds(1);

            v->last_wakeup_ = v->wakeup_;
            v->wakeup_ = now + amt;
            return true;
        }
    }
    return false;
}


}}}

}
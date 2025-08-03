#pragma once

#include "../../../internal/v2/scheduler.h"
#include "../gptimer.h"

#include <estd/port/freertos/wrapper/task.h>

namespace embr { namespace scheduler { namespace esp_idf { inline namespace v1 {

namespace detail {

}

// NOTE: Debatably this guy should be in 'detail'
template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
class gptimer_scheduler : public scheduler::v1::detail::scheduler<Traits, Container>
{
    using timer_type = embr::esp_idf::gptimer;

    // ISR callback helpers
    void callback();
    static void callback(void*);
    static bool alarm_cb(gptimer_handle_t timer,
        const gptimer_alarm_event_data_t* edata,
        void* user_ctx);

    timer_type timer_;
    estd::freertos::wrapper::task task_;

public:
    void init();
    void deinit()
    {
        timer_.del_timer();
    }
};

}}}}

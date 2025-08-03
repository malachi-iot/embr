#pragma once

#include "../../../internal/v2/scheduler.h"
#include "../gptimer.h"

#include <estd/port/freertos/mutex.h>
#include <estd/port/freertos/wrapper/task.h>

namespace embr { namespace scheduler { namespace esp_idf { inline namespace v1 {

namespace detail {

}

// NOTE: Debatably this guy should be in 'detail'
template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
class gptimer_scheduler : public scheduler::v1::detail::scheduler<Traits, Container>
{
    static constexpr const char* TAG = "gptimer_scheduler";

    using base_type = scheduler::v1::detail::scheduler<Traits, Container>;
    using timer_type = embr::esp_idf::gptimer;
    using mutex_type = estd::freertos::mutex<true>;

    // ISR callback helpers
    void callback();
    static void callback(void*);

    bool alarm_cb(const gptimer_alarm_event_data_t* edata);
    static bool alarm_cb(gptimer_handle_t timer,
        const gptimer_alarm_event_data_t* edata,
        void* user_ctx);

    timer_type timer_;
    estd::freertos::wrapper::task task_;

    // Indicate a binary semaphore rather than a mutex, since FreeRTOS doesn't like
    // releasing mutex from ISR
    mutex_type mutex_{true};

    esp_err_t schedule();

public:
    esp_err_t init();
    void deinit()
    {
        timer_.del_timer();
    }
};

}}}}

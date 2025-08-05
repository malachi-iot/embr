#pragma once

#include "../../freertos/v2/scheduler.h"
#include "../gptimer.h"

#include <estd/port/freertos/mutex.h>
#include <estd/port/freertos/wrapper/task.h>

namespace embr { namespace scheduler { namespace esp_idf { inline namespace v1 {

namespace detail {

struct gptimer_context : freertos::detail::scheduler_base2
{
    using mutex_type = estd::freertos::mutex<static_alloc>;

    // Indicate a binary semaphore rather than a mutex, since FreeRTOS doesn't like
    // releasing mutex from ISR
    mutex_type mutex_{true};
    estd::freertos::wrapper::task task_;

    bool alarm_cb(gptimer_handle_t timer,
        const gptimer_alarm_event_data_t* edata);
    static bool IRAM_ATTR alarm_cb(gptimer_handle_t timer,
        const gptimer_alarm_event_data_t* edata,
        void* user_ctx);
};

}

// NOTE: Debatably this guy should be in 'detail'
template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
class gptimer_scheduler : public scheduler::v1::detail::scheduler<Traits, Container>,
    // DEBT: Make this a has-a rather than an is-a
    detail::gptimer_context
{
    static constexpr const char* TAG = "gptimer_scheduler";

    using base_type = scheduler::v1::detail::scheduler<Traits, Container>;
    using typename base_type::process_result;
    using timer_type = embr::esp_idf::gptimer;

    // ISR callback helpers
    void callback();
    static void callback(void*);

    // Troubles. See:
    // https://bitbucket.org/malachib/playground.esp/issues/71/c-templated-method-in-iram
    // https://github.com/espressif/esp-idf/issues/4542
    bool alarm_cb(const gptimer_alarm_event_data_t* edata);
    static bool alarm_cb(gptimer_handle_t timer,
        const gptimer_alarm_event_data_t* edata,
        void* user_ctx);

    timer_type timer_;

    esp_err_t schedule();

public:
    using typename base_type::pointer;

    esp_err_t init();
    void deinit()
    {
        timer_.disable();
        timer_.del_timer();
    }

    esp_err_t start()   { return timer_.start(); }
    esp_err_t stop()    { return timer_.stop(); }

    esp_err_t reschedule(pointer);

    process_result process_one(duration);
};

}}}}

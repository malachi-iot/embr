#pragma once

#include "../../freertos/v2/scheduler.h"
#include "../gptimer.h"

#include <estd/port/freertos/mutex.h>
#include <estd/port/freertos/wrapper/task.h>

namespace embr { namespace scheduler { namespace esp_idf { inline namespace v1 {

namespace detail {

// Pseudo std-compliant clock (instanced, making it non conformant)
struct gptimer_clock
{
    using timer_type = embr::esp_idf::gptimer;
    using duration = estd::chrono::duration<uint64_t, estd::micro>;
    using time_point = estd::chrono::time_point<gptimer_clock, duration>;

    timer_type timer_;

    uint64_t raw_now() const
    {
        uint64_t now;

        ESP_ERROR_CHECK(timer_.get_raw_count(&now));

        return now;
    }

    time_point now() const
    {
        return time_point(duration(raw_now()));
    }
};

struct gptimer_context : freertos::detail::scheduler_base2
{
    using mutex_type = estd::freertos::mutex<static_alloc>;
    using task_type = estd::freertos::wrapper::task;

    // Indicate a binary semaphore rather than a mutex, since FreeRTOS doesn't like
    // releasing mutex from ISR
    mutex_type mutex_{true};
    task_type task_;

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
    using context_type = detail::gptimer_context;
    using typename base_type::process_result;
    using timer_type = embr::esp_idf::gptimer;

    // Subtracts this from 'next', waking alarm up a little early.  This way by the time
    // RTOS task schedules in (process_one) we're closer to the true 'next' time.
    // NOTE: Be careful.  If we beat process_one, paradigm is to cycle to a 2nd process_one.
    // Chances are high the 2nd process_one will catch it.  However, if preload is too high,
    // 2nd process_one will happen quickly enough that you reach a 3rd process one.  That
    // one will wait for a notification which in theory may never come.  Consider adding
    // a counter as a safeguard.
    // DEBT: If we can, heed system clock speed here
    // DEBT: A bringup profiling/tuning phase would be nice
#if CONFIG_IDF_TARGET_ARCH_RISCV
    static constexpr unsigned preload = 6;
#elif CONFIG_IDF_TARGET_ESP32S3
    static constexpr unsigned preload = 30;
#else
    static constexpr unsigned preload = 0;
#endif

    // ISR callback helpers
    void callback();
    static void callback(void*);

    // Troubles. See:
    // https://bitbucket.org/malachib/playground.esp/issues/71/c-templated-method-in-iram
    // https://github.com/espressif/esp-idf/issues/4542
    bool alarm_cb_dormant(const gptimer_alarm_event_data_t* edata);
    static bool alarm_cb_dormant(gptimer_handle_t timer,
        const gptimer_alarm_event_data_t* edata,
        void* user_ctx);

    timer_type timer_;

    esp_err_t schedule();

public:
    using typename base_type::pointer;

    // DEBT: Only for unit tests, external parties ought not to see this
    const timer_type& timer() const { return timer_; }
    const detail::gptimer_clock clock() const { return { timer_ }; }

    esp_err_t init();
    void deinit()
    {
        timer_.disable();
        timer_.del_timer();
    }

    esp_err_t start()   { return timer_.start(); }
    esp_err_t stop()    { return timer_.stop(); }

    esp_err_t reschedule(pointer);

    process_result process_one(duration, BaseType_t* notification_received = nullptr);
};

}}}}

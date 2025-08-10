#pragma once

#include <esp_check.h>
#include <esp_log.h>

#include "scheduler.h"

namespace embr { namespace scheduler { namespace esp_idf { inline namespace v1 {

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
inline void gptimer_scheduler<Traits, Container>::callback()
{

}

// DORMANT
template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
void gptimer_scheduler<Traits, Container>::callback(void* arg)
{
    ((gptimer_scheduler*)arg)->callback();
}

// DORMANT - actual alarm_cb located in timer-scheduler.cpp.  This is only
// around for reference
template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
inline bool gptimer_scheduler<Traits, Container>::alarm_cb_dormant(const gptimer_alarm_event_data_t* edata)
{
    BaseType_t taskWake {};

    SemaphoreHandle_t s = mutex_.native_handle();

    const BaseType_t got_mutex = xSemaphoreTakeFromISR(s, &taskWake);

    if(!got_mutex)
    {
        const gptimer_alarm_config_t alarm_config
        {
            .alarm_count = edata->alarm_value + 20, // Try again 20uS later
            .reload_count {},
            .flags {}
        };

        timer_.set_alarm_action(&alarm_config);
        return true;
    }

    xSemaphoreGiveFromISR(s, &taskWake);

    const uint64_t next = base_type::next();

    const gptimer_alarm_config_t alarm_config
    {
        //.alarm_count = edata->alarm_value + 1000000, // Next alarm in 1s from the current alarm
        .alarm_count = next,
        .reload_count {},
        .flags {}
    };

    timer_.set_alarm_action(&alarm_config);

    return {};
}

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
bool gptimer_scheduler<Traits, Container>::alarm_cb_dormant(gptimer_handle_t timer,
    const gptimer_alarm_event_data_t* edata,
    void* user_ctx)
{
    auto _this = (gptimer_scheduler*)user_ctx;
    assert(_this->timer_ == timer);
    return _this->alarm_cb_dormant(edata);
}



template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
esp_err_t gptimer_scheduler<Traits, Container>::init()
{
    task_ = task_type::current();

    // DEBT: Just for diagnostic, really we ought to be able to use 'this' directly
    detail::gptimer_context* context = this;

    constexpr gptimer_config_t config
    {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
        .intr_priority = 0,
        .flags
        {
            .intr_shared = true,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 4, 0)
            .allow_pd = false,
#endif
            .backup_before_sleep = false,   // deprecated, still present in IDF 6.0
        }
    };

    ESP_RETURN_ON_ERROR(timer_.init(&config), TAG, "Couldn't init timer");

    mutex_.unlock();

    constexpr gptimer_event_callbacks_t cbs
    {
        .on_alarm = context_type::alarm_cb,
    };

    ESP_RETURN_ON_ERROR(timer_.register_event_callbacks(&cbs, context), TAG, "Couldn't set callback");

    ESP_RETURN_ON_ERROR(schedule(), TAG, "Couldn't schedule first item");

    // Just for diagnostic, shouldn't be needed
    //ESP_RETURN_ON_ERROR(timer_.set_raw_count(0), TAG, "Couldn't set counter");

    return timer_.enable();
}

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
inline esp_err_t gptimer_scheduler<Traits, Container>::schedule()
{
    // Could be a possibility, but I anticipate wanting finer control over timings
    //estd::lock_guard<mutex_type> lock(mutex_);

    ESP_LOGV(TAG, "schedule: entry");

    mutex_.lock();
    if(base_type::empty())
    {
        mutex_.unlock();
        ESP_LOGV(TAG, "schedule: exit (empty)");
        return ESP_OK;
    }

    const uint64_t next = base_type::next();

    mutex_.unlock();

    ESP_LOGV(TAG, "schedule: next=%" PRIu64, next);

    const gptimer_alarm_config_t alarm_config
    {
        .alarm_count = next - preload,
        .reload_count {},
        .flags
        {
            .auto_reload_on_alarm = false
        }
    };

    ESP_LOGV(TAG, "schedule: phase 1");

    return timer_.set_alarm_action(&alarm_config);
}


template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
esp_err_t gptimer_scheduler<Traits, Container>::reschedule(pointer item)
{
    [[maybe_unused]]
    bool rescheduled = base_type::reschedule(item);

    return schedule();
}


template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
auto gptimer_scheduler<Traits, Container>::process_one(duration timeout,
    BaseType_t* notification_received) -> process_result
{
    const detail::gptimer_clock clock{timer_};

    process_result r1 = base_type::process_one(clock.raw_now(), mutex_);

    if(r1 != process_result::UNPROCESSED)    return r1;

    uint32_t v;

    // wait on 0 index
    // clear 0 bits on entry
    // clear 0 bits on exit
    [[maybe_unused]]
    BaseType_t r = xTaskNotifyWaitIndexed(0, 0, 0, &v, timeout.count());
    // clearing all bits on entry
    //BaseType_t r = xTaskNotifyWaitIndexed(0, 0xFFFF, 0, &v, timeout.count());
    if(notification_received)   *notification_received = r;

    uint64_t now_us = clock.raw_now();

    r1 = base_type::process_one(now_us, mutex_);

    /*

    //ESP_LOGV(TAG, "process_one: phase 1 now_us=%" PRIu64, now_us);

    mutex_.lock();

    if(!base_type::empty())
    {
        const uint64_t next = base_type::next();
        //now_us = clock.raw_now();

        //ESP_LOGV(TAG, "process_one: now_us=%" PRIu64 ", next=%" PRIu64, now_us, next);

        // DEBT: Consolidate this with above 'schedule' code

        const gptimer_alarm_config_t alarm_config
        {
            .alarm_count = next - preload,
            .reload_count {},
            .flags
            {
                .auto_reload_on_alarm = false
            }
        };

        ESP_ERROR_CHECK(timer_.set_alarm_action(&alarm_config));
    }

    // OK, looks like QEMU has sorta reset the counter when we get here which is technically
    // incorrect behavior.
    //now_us = clock.raw_now();

    //ESP_LOGD(TAG, "process_one: phase 2 now_us=%" PRIu64, now_us);

    mutex_.unlock(); */
    ESP_ERROR_CHECK(schedule());

    return r1;
}


}}}}

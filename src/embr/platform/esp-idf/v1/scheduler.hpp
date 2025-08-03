#pragma once

#include <esp_check.h>

#include "scheduler.h"

namespace embr { namespace scheduler { namespace esp_idf { inline namespace v1 {

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
inline void gptimer_scheduler<Traits, Container>::callback()
{

}

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
void gptimer_scheduler<Traits, Container>::callback(void* arg)
{
    ((gptimer_scheduler*)arg)->callback();
}

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
inline bool gptimer_scheduler<Traits, Container>::alarm_cb(const gptimer_alarm_event_data_t* edata)
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
bool gptimer_scheduler<Traits, Container>::alarm_cb(gptimer_handle_t timer,
    const gptimer_alarm_event_data_t* edata,
    void* user_ctx)
{
    auto _this = (gptimer_scheduler*)user_ctx;
    assert(_this->timer_ == timer);
    return _this->alarm_cb(edata);
}



template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
esp_err_t gptimer_scheduler<Traits, Container>::init()
{
    constexpr gptimer_config_t config
    {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
        .intr_priority = 0,
        .flags
        {
            .intr_shared = true,
            .allow_pd = false,
            .backup_before_sleep = false,
        }
    };

    ESP_RETURN_ON_ERROR(timer_.init(&config), TAG, "Couldn't init timer");

    mutex_.unlock();

    constexpr gptimer_event_callbacks_t cbs
    {
        .on_alarm = alarm_cb,
    };

    ESP_RETURN_ON_ERROR(timer_.register_event_callbacks(&cbs, this), TAG, "Couldn't set callback");

    ESP_RETURN_ON_ERROR(schedule(), TAG, "Couldn't schedule first item");

    return timer_.enable();
}

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
esp_err_t gptimer_scheduler<Traits, Container>::schedule()
{
    mutex_.lock();
    if(base_type::empty())
    {
        return ESP_OK;
        mutex_.unlock();
    }

    const uint64_t next = base_type::next();

    mutex_.unlock();

    const gptimer_alarm_config_t alarm_config
    {
        .alarm_count = next,
        .reload_count {},
        .flags
        {
            .auto_reload_on_alarm = false
        }
    };

    return timer_.set_alarm_action(&alarm_config);
}


}}}}

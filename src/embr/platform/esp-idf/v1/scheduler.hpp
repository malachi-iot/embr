#pragma once

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
bool gptimer_scheduler<Traits, Container>::alarm_cb(gptimer_handle_t timer,
    const gptimer_alarm_event_data_t* edata,
    void* user_ctx)
{
    return {};
}



template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
void gptimer_scheduler<Traits, Container>::init()
{
    constexpr gptimer_config_t config = 
    {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
        .intr_priority = 0,
        .flags
        {
            .intr_shared = true,
            .allow_pd = true,
            .backup_before_sleep = false,
        }
    };

    ESP_ERROR_CHECK(timer_.init(&config));
}


}}}}

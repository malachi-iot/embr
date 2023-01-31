#pragma once

#include <driver/gptimer.h>

namespace embr { namespace esp_idf {

inline namespace v5 {

class Timer
{
    gptimer_handle_t h;

public:
    constexpr Timer() = default;
    constexpr Timer(gptimer_handle_t h) : h{h} {}

    esp_err_t init(const gptimer_config_t* config)
    {
        return gptimer_new_timer(config, &h);
    }

    static esp_err_t new_timer(const gptimer_config_t* config, Timer* timer)
    {
        return timer->init(config);
    }

    esp_err_t del_timer()
    {
        return gptimer_del_timer(h);
    }

    esp_err_t raw_count(uint64_t value) const
    {
        return gptimer_set_raw_count(h, value);
    }

    esp_err_t raw_count(uint64_t* value)
    {
        return gptimer_get_raw_count(h, value);
    }

    esp_err_t enable()
    {
        return gptimer_enable(h);
    }

    esp_err_t disable()
    {
        return gptimer_disable(h);
    }

    esp_err_t start()
    {
        return gptimer_start(h);
    }

    esp_err_t stop()
    {
        return gptimer_stop(h);
    }

    esp_err_t set_alarm_action(const gptimer_alarm_config_t* config)
    {
        return gptimer_set_alarm_action(h, config);
    }

    // "User registered callbacks are expected to be runnable within ISR context" [3]
    esp_err_t register_event_callbacks(const gptimer_event_callbacks_t* cbs, void* user_data = nullptr)
    {
        return gptimer_register_event_callbacks(h, cbs, user_data);
    }

    constexpr operator gptimer_handle_t() const { return h; }
};


}

}}
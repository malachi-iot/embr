#pragma once

#include <driver/gptimer.h>

namespace embr { namespace esp_idf {

inline namespace v5 {

class Timer
{
    gptimer_handle_t h;

public:
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

    esp_err_t set_alarm_action(gptimer_alarm_config_t* config)
    {
        return gptimer_set_alarm_action(h, config);
    }
};


}

}}
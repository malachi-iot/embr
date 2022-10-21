#pragma once

#include <driver/timer.h>

// DEBT: Work out if we want camel case or not, look at other esp_idf specific (non estd flavor)
// code
namespace embr { namespace esp_idf {

struct TimerGroup
{
    timer_group_t group;

    operator timer_group_t () const { return group; }
};


struct Timer
{
    const timer_group_t group;
    const timer_idx_t idx;

    constexpr Timer(timer_group_t group, timer_idx_t idx) :
        group{group},
        idx{idx}
    {

    }

    esp_err_t init(const timer_config_t* config)
    {
        return timer_init(group, idx, config);
    }

    esp_err_t set_alarm(timer_alarm_t alarm_en)
    {
        return timer_set_alarm(group, idx, alarm_en);
    }

    esp_err_t set_alarm_value(uint64_t alarm_value)
    {
        return timer_set_alarm_value(group, idx, alarm_value);
    }

    esp_err_t set_counter_value(uint64_t load_val)
    {
        return timer_set_counter_value(group, idx, load_val);
    }

    esp_err_t start()
    {
        return timer_start(group, idx);
    }

    esp_err_t pause()
    {
        return timer_pause(group, idx);
    }
    
    void enable_alarm_in_isr()
    {
        timer_group_enable_alarm_in_isr(group, idx);
    }

    void enable_intr()
    {
        timer_enable_intr(group, idx);
    }

    void disable_intr()
    {
        timer_disable_intr(group, idx);
    }
};

}}
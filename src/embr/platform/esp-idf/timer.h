// Wrapper for general purpose timer
// https://docs.espressif.com/projects/esp-idf/en/v4.4.2/esp32/api-reference/peripherals/timer.html
// Different than and does not wrap slightly higher level timer component:
// https://docs.espressif.com/projects/esp-idf/en/v4.4.2/esp32/api-reference/system/esp_timer.html
// NOTE: v5 seems to be changing API to 'gptimer_'
#pragma once

#include <new>

#include <estd/string.h>

#include <driver/timer.h>

namespace embr { namespace esp_idf {

inline namespace v4 {

struct TimerGroup
{
    timer_group_t group;

    operator timer_group_t () const { return group; }
};


struct Timer
{
    const timer_group_t group;
    const timer_idx_t idx;

    // ++ EXPERIMENTAL
    // Typically 80Mhz, but can be reconfigured
    static constexpr unsigned apb_hz() { return APB_CLK_FREQ; }
// still some mysteries with version_finder.cmake's defines showing up
// By now it's fine to use Espressif's official version of this, so we'll do so
//#if ESTD_IDF_VER >= ESTD_BUILD_IDF_VER(5, 0, 0, 0)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    static constexpr unsigned base_clock_hz() { return apb_hz(); }
#else
    static constexpr unsigned base_clock_hz() { return TIMER_BASE_CLK; }
#endif
    // --

    constexpr Timer(timer_group_t group, timer_idx_t idx) :
        group{group},
        idx{idx}
    {

    }

    Timer(const Timer& copy_from) = default;
    Timer& operator =(const Timer& copy_from)
    {
        return * new (this) Timer(copy_from);
    }

    esp_err_t init(const timer_config_t* config)
    {
        return timer_init(group, idx, config);
    }

    esp_err_t deinit()
    {
        return timer_deinit(group, idx);
    }

    inline esp_err_t get_alarm_value(uint64_t* alarm_val) const
    {
        return timer_get_alarm_value(group, idx, alarm_val);
    }

    inline esp_err_t get_config(timer_config_t* config)
    {
        return timer_get_config(group, idx, config);
    }

    inline esp_err_t get_counter_value(uint64_t* timer_val) const
    {
        return timer_get_counter_value(group, idx, timer_val);
    }

    inline uint64_t get_counter_value_in_isr() const
    {
        return timer_group_get_counter_value_in_isr(group, idx);
    }

    esp_err_t set_alarm(timer_alarm_t alarm_en)
    {
        return timer_set_alarm(group, idx, alarm_en);
    }

    esp_err_t set_alarm_value(uint64_t alarm_value)
    {
        return timer_set_alarm_value(group, idx, alarm_value);
    }

    void set_alarm_value_in_isr(uint64_t alarm_value)
    {
        timer_group_set_alarm_value_in_isr(group, idx, alarm_value);
    }

    /// @brief  Sets initial counter value
    /// @param load_val 
    /// @return 
    esp_err_t set_counter_value(uint64_t load_val)
    {
        return timer_set_counter_value(group, idx, load_val);
    }

    void set_counter_enable_in_isr(timer_start_t counter_en)
    {
        timer_group_set_counter_enable_in_isr(group, idx, counter_en);
    }

    esp_err_t set_divider(uint32_t divider)
    {
        return timer_set_divider(group, idx, divider);
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

    esp_err_t enable_intr()
    {
        return timer_enable_intr(group, idx);
    }

    esp_err_t disable_intr()
    {
        return timer_disable_intr(group, idx);
    }

    esp_err_t isr_callback_add(timer_isr_t isr_handler, void* arg, int intr_alloc_flags)
    {
        return timer_isr_callback_add(group, idx, isr_handler, arg, intr_alloc_flags);
    }
};


// DEBT: Would like to do a const_string here, but estd conversion/availability
// for a layer1 const_string seems to be impacted
inline estd::layer1::string<8> to_string(const Timer& timer)
{
    estd::layer1::string<8> timer_str;

    timer_str += estd::to_string((int)timer.group);
    timer_str += ':';
    timer_str += estd::to_string((int)timer.idx);

    return timer_str;
}

}

}}
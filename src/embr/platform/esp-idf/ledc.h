/**
 * 15JUN23 Imported and adapted from bronze-star
 */
#pragma once

extern "C" {

#include <driver/ledc.h>

}

namespace embr { namespace esp_idf {

class ledc
{
    ledc_channel_config_t config_;

public:
    constexpr ledc() = default;
    constexpr ledc(ledc_channel_config_t&& config) :
        config_(config) {}

    esp_err_t set_duty(uint32_t duty)
    {
        return ledc_set_duty(config_.speed_mode, config_.channel, duty);
    }

    esp_err_t update_duty()
    {
        return ledc_update_duty(config_.speed_mode, config_.channel);
    }

    esp_err_t set_duty_and_update(uint32_t duty, uint32_t hpoint)
    {
        return ledc_set_duty_and_update(config_.speed_mode, config_.channel, duty, hpoint);
    }

    esp_err_t set_duty_and_update(uint32_t duty)
    {
        return ledc_set_duty_and_update(config_.speed_mode, config_.channel, duty, config_.hpoint);
    }

    esp_err_t fade_start(ledc_fade_mode_t fade_mode = LEDC_FADE_NO_WAIT)
    {
        return ledc_fade_start(config_.speed_mode, config_.channel, fade_mode);
    }

    esp_err_t set_fade(uint32_t duty, ledc_duty_direction_t fade_direction, uint32_t step_num,
        uint32_t duty_cycle_num, uint32_t duty_scale)
    {
        return ledc_set_fade(config_.speed_mode, config_.channel, duty, 
            fade_direction, step_num, duty_cycle_num, duty_scale);
    }

    esp_err_t set_fade_with_time(uint32_t target_duty, int max_fade_time_ms) const
    {
        return ledc_set_fade_with_time(config_.speed_mode, config_.channel, target_duty,
            max_fade_time_ms);
    }

    esp_err_t set_fade_time_and_start(uint32_t target_duty, uint32_t max_fade_time_ms, 
        ledc_fade_mode_t fade_mode = LEDC_FADE_NO_WAIT) const
    {
        return ledc_set_fade_time_and_start(config_.speed_mode, config_.channel,
            target_duty, max_fade_time_ms, fade_mode);
    }

    esp_err_t config() const
    {
        return ledc_channel_config(&config_);
    }

    esp_err_t config(const ledc_channel_config_t& c)
    {
        config_ = c;
        return config();
    }

    esp_err_t stop(uint32_t idle_level)
    {
        return ledc_stop(config_.speed_mode, config_.channel, idle_level);
    }
};


}}


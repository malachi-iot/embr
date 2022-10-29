#pragma once

#include <driver/gpio.h>


namespace embr { namespace esp_idf {

// DEBT: Other wrappers are capitalized
class gpio
{
    const gpio_num_t pin_;

public:
    constexpr gpio(gpio_num_t pin) : pin_{pin} {}
    gpio(const gpio& copy_from) = default;
    gpio& operator=(const gpio& copy_from)
    {
        // DEBT: This doesn't pass the smell test
        return * new (this) gpio(copy_from);
    }

    esp_err_t reset() const
    {
        return gpio_reset_pin(pin_);
    }

    inline int level() const
    {
        return gpio_get_level(pin_);
    }

    inline esp_err_t level(uint32_t v) const
    {
        return gpio_set_level(pin_, v);
    }

    esp_err_t set_direction(gpio_mode_t mode) const
    {
        return gpio_set_direction(pin_, mode);
    }

    esp_err_t set_pull_mode(gpio_pull_mode_t pull) const
    {
        return gpio_set_pull_mode(pin_, pull);
    }

    constexpr operator gpio_num_t () const { return pin_; }
};

}}
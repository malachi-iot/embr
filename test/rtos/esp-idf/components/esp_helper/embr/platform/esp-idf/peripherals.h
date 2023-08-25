#pragma once

// EXPERIMENTAL

namespace embr { namespace esp_idf { inline namespace exp { namespace R {

struct gpio_input {};

struct gpio_output {};

struct rmt_output : gpio_output {};

struct button : gpio_input {};

struct ws2812 : rmt_output {};

struct led : gpio_output {};

template <unsigned pin_, class Trait>
struct gpio
{
    static constexpr gpio_num_t pin = (gpio_num_t)pin_;
    using traits = Trait;
};

}}}}
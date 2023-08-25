#pragma once

// EXPERIMENTAL

namespace embr { namespace esp_idf { inline namespace exp { namespace R {

struct gpio_input {};

struct gpio_output {};

struct rmt_output : gpio_output {};

struct button : gpio_input {};

struct rgb_led : gpio_output {};

template <unsigned I>
struct indexed_gpio_output : gpio_output
{
    static constexpr const unsigned index = I;
};

template <unsigned I>
struct spi_cs : indexed_gpio_output<I> {};

template <unsigned I>
struct spi_mosi : indexed_gpio_output<I> {};

template <unsigned I>
struct spi_miso : indexed_gpio_output<I> {};

template <unsigned I>
struct spi_clk : indexed_gpio_output<I> {};

struct ws2812 :
    rgb_led,
    rmt_output {};

struct led : gpio_output {};

struct spi_device {};

struct GC9107 : spi_device {};

template <unsigned pin_, class Trait>
struct gpio
{
    static constexpr gpio_num_t pin = (gpio_num_t)pin_;
    using trait = Trait;
};

}}}}
#pragma once

#include <driver/gpio.h>

#include "selector.h"

// EXPERIMENTAL
// DEBT: Document why I chose R - I think because Android liked that for static resources
// DEBT: Most of this can move out of esp_idf specific area

namespace embr { namespace esp_idf { inline namespace exp { namespace R {

enum class colors
{
    black = 0x000000,
    red = 0xFF0000,
    green = 0x00FF00,
    blue = 0x0000FF,
    yellow = 0xFFFF00
};

constexpr const char* to_string(colors c)
{
    switch(c)
    {
        case colors::green:     return "green";
        case colors::blue:      return "blue";
        case colors::red:       return "red";
        case colors::yellow:    return "yellow";
        
        default:            return "undefined";
    }
};


// Use for simpler scenarios where it's a mere eval to a bool or false without
// need of index
template <template <class> class E>
struct passthrough_selector
{
    template <class T_j, size_t>
    using evaluator = E<T_j>;
};

namespace color {
template <colors c>
using trait = estd::integral_constant<colors, c>;

using red = trait<colors::red>;
using blue = trait<colors::blue>;
using green = trait<colors::green>;
using yellow = trait<colors::yellow>;

template <class>    struct is_trait : estd::false_type {};
template <colors c> struct is_trait<trait<c>> : estd::true_type {};

using selector = passthrough_selector<is_trait>;

};

namespace trait {

// For status indicators, namely a convention chosen for which LED represents general status
// (i.e. usually 13 for Arduino)
struct status {};

}


struct input {};

struct output {};

// We differenciate 'gpio' as open to use by user and not by a specific peripheral
struct gpio_input : input {};

struct gpio_output : output {};

// Indicates mux/peripheral can cooexist with others on the same pin(s)
struct unreserved {};

struct analog_input : input {};

struct rmt_output : output {};

// Buttons seem to coexist with other inputs sometimes, so making this a gpio
struct button : gpio_input {};

struct rgb_led : output {};

template <size_t I, class Tag = void>
struct group : estd::integral_constant<size_t, I> {};

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

struct can_tx : output {};

struct can_rx : output {};

struct ws2812 :
    rgb_led,
    rmt_output {};

struct led :
    unreserved,
    gpio_output {};

struct battery_voltage : analog_input {};

struct spi_device {};

struct GC9107 : spi_device {};

struct ALS_PT19 : analog_input {};

template <unsigned pin_, class Trait, class ...Traits>
struct mux
{
    static constexpr gpio_num_t pin = (gpio_num_t)pin_;
    using trait = Trait;

    using traits = estd::variadic::types<Trait, Traits...>;

    template <class E>
    using select = typename traits::selector<E>;

    template <class E>
    using single = typename select<E>::single::type;
};


}}}}
#pragma once

#include <estd/type_traits.h>

#include <driver/gpio.h>

// EXPERIMENTAL

namespace embr { namespace esp_idf { inline namespace exp { namespace R {

enum class colors
{
    black = 0x000000,
    red = 0xFF0000,
    green = 0x00FF00,
    blue = 0x0000FF,
    yellow = 0xFFFF00
};

namespace color {
template <colors c>
using trait = estd::integral_constant<colors, c>;

using red = trait<colors::red>;
using blue = trait<colors::blue>;
using yellow = trait<colors::yellow>;

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
};


// Like is_same_selector, but instead evaluates all of ...Types to see
// if one of them is present
// DEBT: I'm thinking we can name this "any_selector" - but not sure
// DEBT: Once we work that out, put this into 'estd'
template <class ...Types>
struct is_in_selector
{
    using requested_types = estd::variadic::types<Types...>;


    template <class T_j>
    using helper2 = typename requested_types::selector<estd::internal::is_same_selector<T_j> >;

    template <class T_j, size_t>
    using evaluator = estd::bool_constant<helper2<T_j>::size() != 0>;
};

template <class ...Traits>
struct traits_selector
{
    template <class T_j>
    using helper = typename T_j::traits::selector<is_in_selector<Traits...> >;

    template <class T_j, size_t>
    using evaluator = estd::bool_constant<helper<T_j>::size() == sizeof...(Traits)>;
};

}}}}
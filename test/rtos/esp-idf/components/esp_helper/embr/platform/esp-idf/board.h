#pragma once

#include <sdkconfig.h>

#include <estd/internal/variadic.h>

// NOTE: Don't go *too* crazy with this, because if we do we might be better
// served porting this to modm

namespace embr { namespace esp_idf {

struct board_traits
{
    struct gpio
    {
#ifdef CONFIG_BOARD_ESP32_WEMOS_MINI32
        static constexpr unsigned boot_button = 0;
        static constexpr unsigned user_button = 0;
        static constexpr unsigned status_led = 2;
#elif defined(CONFIG_BOARD_ESP32C3_SEEED_XIAO)
        static constexpr unsigned boot_button = 9;
        static constexpr unsigned user_button = 9;
#elif defined(BOARD_ESP32_ESP_WROVER_KIT_41)
        static constexpr unsigned rgb_g_led = 0;
        static constexpr unsigned rgb_r_led = 2;
        static constexpr unsigned rgb_b_led = 4;
        static constexpr unsigned status_led = rgb_r_led;
#else
#endif
    };
};

}}
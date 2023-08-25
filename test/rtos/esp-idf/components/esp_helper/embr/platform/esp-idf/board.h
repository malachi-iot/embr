#pragma once

#include <sdkconfig.h>

#include <estd/internal/variadic.h>

// NOTE: Don't go *too* crazy with this, because if we do we might be better
// served porting this to modm

namespace embr { namespace esp_idf {

#define EMBR_BOARD_LED_TYPE_NONE        0
#define EMBR_BOARD_LED_TYPE_SIMPLE      1

#if defined(CONFIG_BOARD_ESP32_WEMOS_MINI32) || \
    defined(CONFIG_BOARD_ESP32_ESP_WROVER_KIT_41)
#define FEATURE_EMBR_BOARD_STATUS_LED EMBR_BOARD_LED_TYPE_SIMPLE
#endif


struct board_traits
{
#ifdef CONFIG_BOARD_ESP32_WEMOS_MINI32
    static constexpr const char* vendor = "Wemos";
    static constexpr const char* name = "Mini32";
#elif defined(CONFIG_BOARD_ESP32_ESP_WROVER_KIT_41)
    static constexpr const char* vendor = "Espressif";
    static constexpr const char* name = "WROVER Kit 4.1";
#elif defined(CONFIG_BOARD_ESP32C3_DEVKITM_1)
    static constexpr const char* vendor = "Espressif";
    static constexpr const char* name = "DevKitM-1";
#else
    static constexpr const char* vendor = "Unspecified";
    static constexpr const char* name = "Generic";
#endif
    struct gpio
    {
#ifdef CONFIG_BOARD_ESP32_WEMOS_MINI32
        static constexpr unsigned boot_button = 0;
        static constexpr unsigned user_button = 0;
        static constexpr unsigned status_led = 2;
#elif defined(CONFIG_BOARD_ESP32C3_SEEED_XIAO)
        static constexpr unsigned boot_button = 9;
        static constexpr unsigned user_button = 9;
#elif defined(CONFIG_BOARD_ESP32_ESP_WROVER_KIT_41)
        static constexpr unsigned rgb_g_led = 0;
        static constexpr unsigned rgb_r_led = 2;
        static constexpr unsigned rgb_b_led = 4;
        static constexpr unsigned status_led = rgb_r_led;
#elif defined(CONFIG_BOARD_ESP32S3_UM_FEATHERS3)
#elif defined(CONFIG_BOARD_ESP32C3_DEVKITM_1)
        static constexpr unsigned boot_button = 9;
        static constexpr unsigned user_button = 9;
#else
#endif
    };
};

}}
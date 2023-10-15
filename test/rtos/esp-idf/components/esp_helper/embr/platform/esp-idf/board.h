#pragma once

#include <sdkconfig.h>

#include <estd/internal/variadic.h>

// NOTE: Not yet used, but feels appropriate to bring it in
#include <embr/platform/esp-idf/traits.h>

#include "peripherals.h"

// NOTE: Don't go *too* crazy with this, because if we do we might be better
// served porting this to modm

namespace embr { namespace esp_idf {

#define EMBR_BOARD_LED_TYPE_NONE        0
#define EMBR_BOARD_LED_TYPE_SIMPLE      1

#if defined(CONFIG_BOARD_ESP32_WEMOS_MINI32) || \
    defined(CONFIG_BOARD_ESP32_ESP_WROVER_KIT_41) || \
    defined(CONFIG_BOARD_ESP32S3_UM_FEATHERS3)
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
#elif defined(CONFIG_BOARD_ESP32C3_SEEED_XIAO)
    static constexpr const char* vendor = "Seeed Studio";
    static constexpr const char* name = "XIAO ESP32C3";
#elif defined(CONFIG_BOARD_ESP32S3_UM_FEATHERS3)
    static constexpr const char* vendor = "Unexpected Maker";
    static constexpr const char* name = "Feather S3";
#elif defined(CONFIG_BOARD_ESP32S3_REJSACAN_V3_1)
    static constexpr const char* vendor = "Magnus Thome";
    static constexpr const char* name = "RejsaCAN v3.1";
#elif defined(CONFIG_BOARD_ESP32S3_LILYGO_T_QT_PRO)
    static constexpr const char* vendor = "LilyGO";
    static constexpr const char* name = "QT Pro";
#else
    static constexpr const char* vendor = "Unspecified";
    static constexpr const char* name = "Generic";
#endif

    // NOTE: Attempt to use io mux below instead
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
        static constexpr unsigned boot_button = 0;
        static constexpr unsigned user_button = 0;
        static constexpr unsigned status_led = 13;
#elif defined(CONFIG_BOARD_ESP32C3_DEVKITM_1)
        static constexpr unsigned boot_button = 9;
        static constexpr unsigned user_button = 9;
#else
#endif
    };

    using io = estd::variadic::types<
#if defined(CONFIG_BOARD_ESP32_WEMOS_MINI32)
        R::mux<0, R::button>,
        R::mux<2, R::led, R::color::blue, R::trait::status>
#elif defined(CONFIG_BOARD_ESP32_ESP_WROVER_KIT_41)
        R::mux<0, R::button>,
        R::mux<0, R::led, R::color::red,   R::group<0>, R::trait::status>,
        R::mux<2, R::led, R::color::green, R::group<0>>,
        R::mux<4, R::led, R::color::blue,  R::group<0>>
#elif defined(CONFIG_BOARD_ESP32C3_DEVKITM_1)
        R::mux<8, R::ws2812>,
        R::mux<9, R::button>
#elif defined(CONFIG_BOARD_ESP32C3_SEEED_XIAO)
        R::mux<9, R::button>
#elif defined(CONFIG_BOARD_ESP32S3_UM_FEATHERS3)
        R::mux<40, R::ws2812>,
        R::mux<13, R::led, R::color::blue, R::trait::status>,
        R::mux<4, R::ALS_PT19>,
        R::mux<2, R::battery_voltage>,
        R::mux<0, R::button>
#elif defined(CONFIG_BOARD_ESP32S3_LILYGO_T_QT_PRO)
        R::mux<0, R::button>,
        R::mux<2, R::spi_mosi<0>>,
        R::mux<3, R::spi_clk<0>>,
        R::mux<5, R::spi_cs<0>>,
        R::mux<47, R::button>
#elif defined(CONFIG_BOARD_ESP32S3_REJSACAN_V3_1)
        R::mux<0, R::button>,
        R::mux<10, R::led, R::color::blue, R::trait::status>,
        R::mux<11, R::led, R::color::yellow>,
        R::mux<13, R::can_rx>,
        R::mux<14, R::can_tx>
#endif
        >;

    using peripherals = estd::variadic::types<
#if defined(CONFIG_BOARD_ESP32S3_LILYGO_T_QT_PRO)
        R::GC9107
#elif defined(CONFIG_BOARD_ESP32_ESP_WROVER_KIT_41)
#endif
        >;
};

}}
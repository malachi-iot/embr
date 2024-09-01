#pragma once

#include "../base.h"

namespace embr { namespace esp_idf { namespace traits { inline namespace v1 {

struct board : v1::internal::board
{
#if CONFIG_BOARD_ESP32S3_SEEED_XIAO
    static constexpr const char* vendor = "Seeed Studio";
    static constexpr const char* name = "XIAO";
#elif BOARD_ESP32S3_ADAFRUIT_FEATHER_REVTFT
    static constexpr const char* vendor = "Adafruit";
    static constexpr const char* name = "Feather Reverse TFT";
#elif CONFIG_BOARD_ESP32S3_FREENOVE
    static constexpr const char* vendor = "FreeNove";
    static constexpr const char* name = "S3";
#elif CONFIG_BOARD_ESP32S3_UM_FEATHERS3
    static constexpr const char* vendor = "Unexpected Maker";
    static constexpr const char* name = "FeatherS3";
#elif CONFIG_BOARD_ESP32S3_REJSACAN_V3_1
    static constexpr const char* vendor = "Magnus Thome";
    static constexpr const char* name = "RejsaCAN v3.1";
#elif CONFIG_BOARD_ESP32S3_LILYGO_T_QT_PRO
    static constexpr const char* vendor = "LilyGO";
    static constexpr const char* name = "QT Pro";
#elif BOARD_ESP32S3_LILYGO_T5_EINK_2_3
    static constexpr const char* vendor = "LilyGO";
    static constexpr const char* name = "T5 EInk 2.3";
#endif

    using io = mux<
#if defined(CONFIG_BOARD_ESP32S3_SEEED_XIAO)
        R::mux<21, R::led, R::color::orange, R::trait::status>,
        R::mux<1, R::arduino<0>>,
        R::mux<0, R::button>
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
#endif
        >;
};

}}}}
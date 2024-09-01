#pragma once

#include "../base.h"

namespace embr { namespace esp_idf { namespace traits { inline namespace v1 {

struct board : v1::internal::board
{
#if CONFIG_BOARD_ESP32C6_WAVESHARE_DEVKIT
    static constexpr const char* vendor = "WaveShare";
    static constexpr const char* name = "DevKit";
#elif CONFIG_BOARD_ESP32C6_SEEED_XIAO
    static constexpr const char* vendor = "Seeed Studio";
    static constexpr const char* name = "XIAO";

    using io = mux<
        R::mux<9, R::arduino<9>, R::button>,        // DEBT: Not yet verified
        R::mux<15, R::led, R::color::yellow>
        >;
#endif
};

}}}}

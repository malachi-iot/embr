#pragma once

#include "../base.h"

namespace embr { namespace esp_idf { namespace traits { inline namespace v1 {

struct board : v1::internal::board
{
#if CONFIG_BOARD_ESP32C3_SEEED_XIAO
    static constexpr const char* vendor = "Seeed Studio";
    static constexpr const char* name = "XIAO";

    using io = mux<
        R::mux<9, R::arduino<9>, R::button>,
        R::mux<2, R::arduino<0>>
        >;

#elif CONFIG_BOARD_ESP32C3_DEVKITM_1
    static constexpr const char* vendor = "Espressif";
    static constexpr const char* name = "DevKitM-1";

    using io = mux<
        R::mux<8, R::ws2812>,
        R::mux<9, R::button>>;

#endif
};

}}}}
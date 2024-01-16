#pragma once

#include "../base.h"

namespace embr { namespace esp_idf { namespace traits { inline namespace v1 {

struct board : v1::internal::board
{
#if CONFIG_BOARD_ESP32_WEMOS_MINI32
    static constexpr const char* vendor = "Wemos";
    static constexpr const char* name = "Mini32";

    using io = mux<
        R::mux<0, R::button>,
        R::mux<2, R::led, R::color::blue, R::trait::status>>;

#elif CONFIG_BOARD_ESP32_ESP_WROVER_KIT_41
    static constexpr const char* vendor = "Espressif";
    static constexpr const char* name = "WROVER Kit 4.1";

    using io = mux<
        R::mux<0, R::button>,
        R::mux<0, R::led, R::color::red,   R::group<0>, R::trait::status>,
        R::mux<2, R::led, R::color::green, R::group<0>>,
        R::mux<4, R::led, R::color::blue,  R::group<0>>>;
#endif
};

}}}}
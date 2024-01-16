#pragma once

#include "../base.h"

namespace embr { namespace esp_idf { namespace traits { inline namespace v1 {

struct board : v1::internal::board
{
#if CONFIG_BOARD_ESP32C6_WAVESHARE_DEVKIT
    static constexpr const char* vendor = "WaveShare";
    static constexpr const char* name = "DevKit";
#endif
};

}}}}

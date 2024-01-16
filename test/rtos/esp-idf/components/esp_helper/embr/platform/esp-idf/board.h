#pragma once

#include <sdkconfig.h>

#include <estd/internal/variadic.h>

// NOTE: Not yet used, but feels appropriate to bring it in
#include <embr/platform/esp-idf/traits.h>

#include "peripherals.h"
#include "board/traits.h"
#include "board/overlay.h"

namespace embr { namespace esp_idf {

#define EMBR_BOARD_LED_TYPE_NONE        0
#define EMBR_BOARD_LED_TYPE_SIMPLE      1

#if defined(CONFIG_BOARD_ESP32_WEMOS_MINI32) || \
    defined(CONFIG_BOARD_ESP32_ESP_WROVER_KIT_41) || \
    defined(CONFIG_BOARD_ESP32S3_UM_FEATHERS3)
#define FEATURE_EMBR_BOARD_STATUS_LED EMBR_BOARD_LED_TYPE_SIMPLE
#endif

using board_traits = traits::v1::board;

}}
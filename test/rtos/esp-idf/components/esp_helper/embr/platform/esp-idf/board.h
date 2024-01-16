#pragma once

#include <sdkconfig.h>

#include <estd/internal/variadic.h>

// NOTE: Not yet used, but feels appropriate to bring it in
#include <embr/platform/esp-idf/traits.h>

#include "peripherals.h"

#ifdef CONFIG_IDF_TARGET_ESP32
#include "board/esp32/traits.h"
#elif CONFIG_IDF_TARGET_ESP32C3
#include "board/esp32c3/traits.h"
#elif CONFIG_IDF_TARGET_ESP32C6
#include "board/esp32c6/traits.h"
#elif CONFIG_IDF_TARGET_ESP32S3
#include "board/esp32s3/traits.h"
#endif

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
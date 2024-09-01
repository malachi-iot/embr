#pragma once

#ifdef CONFIG_IDF_TARGET_ESP32
#include "esp32/traits.h"
#elif CONFIG_IDF_TARGET_ESP32C3
#include "esp32c3/traits.h"
#elif CONFIG_IDF_TARGET_ESP32C6
#include "esp32c6/traits.h"
#elif CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3/traits.h"
#elif CONFIG_IDF_TARGET_ESP32P4
#include "esp32p4/traits.h"
#endif

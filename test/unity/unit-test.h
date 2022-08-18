#pragma once

#include <unity.h>

#if defined(ESP_PLATFORM) && !defined(ARDUINO)
#define ESP_IDF_TESTING
#endif

void test_bits();

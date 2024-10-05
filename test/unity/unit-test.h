#pragma once

#include <unity.h>

// TODO: Finds it, but Catch REQUIRE makes it mad.  #ifdef that out
//#include "../catch/test-data.h"
//#include "../catch/test-observer.h"

#if defined(ESP_PLATFORM) && !defined(ARDUINO)
#define ESP_IDF_TESTING
#endif

void test_bits();
void test_delegate_queue();
void test_lwip();
void test_observer();
void test_word();

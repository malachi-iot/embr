#include <pico/stdio_usb.h>

#include <unity.h>

#include <unit-test.h>

void setUp() {}

void tearDown() {}

int main()
{
    stdio_init_all();

    UNITY_BEGIN();
    test_bits();
    test_lwip();
    test_word();
    UNITY_END();
}
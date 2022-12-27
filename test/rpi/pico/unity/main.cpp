#include <estd/ostream.h>

#include <pico/cyw43_arch.h>
#include <pico/stdio_usb.h>

#include <unity.h>

#include <unit-test.h>

#include <test/support.h>

void setUp()
{
    //cyw43_arch_enable_sta_mode();
}

void tearDown() {}

int main()
{
    test::v1::init();

    UNITY_BEGIN();
    test_bits();
    test_lwip();
    test_word();
    UNITY_END();
}
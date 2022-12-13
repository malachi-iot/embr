#include <estd/ostream.h>

#include <pico/cyw43_arch.h>
#include <pico/stdio_usb.h>

#include <unity.h>

#include <unit-test.h>

static estd::pico_ostream clog(stdio_usb);

void setUp()
{
    //cyw43_arch_enable_sta_mode();
}

void tearDown() {}

int main()
{
    stdio_init_all();

    clog << "starting..." << estd::endl;

    if (cyw43_arch_init())
    {
        clog << "setUp: failed to initialise" << estd::endl;
        return -1;
    }

    UNITY_BEGIN();
    test_bits();
    test_lwip();
    test_word();
    UNITY_END();
}
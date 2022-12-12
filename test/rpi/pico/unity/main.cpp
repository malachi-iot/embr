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
    // DEBT: unity-tests-lib may need to be an interface lib for this to work so it can pick up
    // -D defines and enable lwip
    //test_lwip();
    test_word();
    UNITY_END();
}
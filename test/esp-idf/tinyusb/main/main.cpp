

#include <embr/platform/tinyusb/streambuf.h>

extern "C" void app_main(void)
{
    tud_cdc_n_write_char(0, 0);
}

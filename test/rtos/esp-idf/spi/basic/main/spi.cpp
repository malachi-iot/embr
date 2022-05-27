#include <estd/chrono.h>
#include <estd/thread.h>

#include <embr/platform/esp-idf/spi.h>

#include <esp_log.h>
#include <driver/gpio.h>

#include "spi.h"

#define CPP_MODE 0

using namespace embr::esp_idf;
using namespace estd::chrono;

static spi::bus bus(LCD_HOST);
static spi::device device;

#if CPP_MODE
namespace cpp {

#include "spi_inc.h"

}

#else

extern "C" void copy_to_buscfg(spi_bus_config_t* copy_to);
extern "C" void copy_to_devcfg(spi_device_interface_config_t* copy_to);

#endif


// this config style generates a lot of warnings
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61489
// https://en.cppreference.com/w/cpp/language/aggregate_initialization
// indicates that they are still zero-initialized.
// so in the short term, we have no worries
// looks like there may be a compiler bug
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82283#c6 because we should be getting less
// warnings if we bookend the fields.  Perhaps things aren't getting zero initialized then also
// due to a compiler bug?  Something to check out.
void spi_init()
{
    spi_bus_config_t buscfg;

#if CPP_MODE
    cpp::copy_to_buscfg(&buscfg);
    bus.initialize(buscfg, SPI_DMA_CH_AUTO);
#else
    copy_to_buscfg(&buscfg);
    ESP_ERROR_CHECK(spi_bus_initialize(bus, &buscfg, SPI_DMA_CH_AUTO));
#endif

    spi_device_interface_config_t devcfg;

#if CPP_MODE
    cpp::copy_to_devcfg(&devcfg);
#else
    copy_to_devcfg(&devcfg);
#endif

    // Have to set it to 1 for it to operate as if it was in '0' mode
    //devcfg.mode = 1;
    //devcfg.duty_cycle_pos = 0;
    //devcfg.cs_ena_pretrans = 0;

    //devcfg.duty_cycle_pos = 64;

#if CPP_MODE
    device = bus.add(devcfg);
#else
    spi_device_handle_t handle;
    ESP_ERROR_CHECK(spi_bus_add_device(bus, &devcfg, &handle));
    device = handle;
#endif
}

static const char msg[] {"Hello World!"};
static int counter = 0;

void spi_loop()
{
    static const char* TAG = "spi_loop";

    spi_master_ostreambuf out(device);

    estd::this_thread::sleep_for(milliseconds(250));

    char c = msg[counter];

    int result = out.sputc(c);

    ESP_LOGI(TAG, "sputc('%c') result = %d", c, result);

    if(++counter == sizeof(msg))
        counter = 0;
}
#include <estd/chrono.h>
#include <estd/thread.h>

#include <embr/platform/esp-idf/spi.h>

#include <esp_log.h>
#include <driver/gpio.h>

#include "spi.h"

using namespace embr::esp_idf;

static spi::bus bus(LCD_HOST);
static spi::device device;

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
    spi_bus_config_t buscfg={
        .mosi_io_num=PIN_NUM_MOSI,
        .miso_io_num=PIN_NUM_MISO,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=PARALLEL_LINES*320*2+8,

        // bookend
        .intr_flags=0
    };

    spi_device_interface_config_t devcfg={
        .mode=0,                                //SPI mode 0
        .clock_speed_hz=1*1000*1000,            //Clock out at 1 MHz
        .spics_io_num=PIN_NUM_CS,               //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time

        // bookend
        .post_cb=nullptr
    };

    bus.initialize(buscfg, SPI_DMA_CH_AUTO);
    device = bus.add(devcfg);
}


void spi_loop()
{
    spi_master_ostreambuf out(device);
}
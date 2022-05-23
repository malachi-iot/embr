#include <estd/chrono.h>
#include <estd/thread.h>

//#include <chrono>

#include <embr/platform/esp-idf/spi.h>

#include "spi.h"

#include <esp_log.h>
#include <driver/gpio.h>

using namespace embr::esp_idf;

//This function is called (in irq context!) just before a transmission starts. It will
//set the D/C line to the value indicated in the user field.
void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc=(int)t->user;
    gpio_set_level((gpio_num_t)PIN_NUM_DC, dc);
}

static spi::bus bus(LCD_HOST);
static spi::device* device;


void spi_init()
{
    esp_err_t ret;

    spi_bus_config_t buscfg={
        .mosi_io_num=PIN_NUM_MOSI,
        .miso_io_num=PIN_NUM_MISO,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=PARALLEL_LINES*320*2+8
    };
    spi_device_interface_config_t devcfg={
        .mode=0,                                //SPI mode 0
#ifdef CONFIG_LCD_OVERCLOCK
        .clock_speed_hz=26*1000*1000,           //Clock out at 26 MHz
#else
        .clock_speed_hz=10*1000*1000,           //Clock out at 10 MHz
#endif
        .spics_io_num=PIN_NUM_CS,               //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
        .pre_cb=lcd_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
    };
    //Initialize the SPI bus
    ret=bus.initialize(buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    static spi::device _device(bus, devcfg);
    // DEBT: Clunky, but will get us through the short term OK
    device = &_device;
}

void spi_loop()
{
    spi_master_ostreambuf out(*device);
    spi_master_istreambuf in(*device);

    // Surprisingly, these are not available
    //using namespace std::chrono_literals;

    // This one is not ready yet
    //using namespace estd::literals::chrono_literals;
    constexpr estd::chrono::milliseconds delay(1000);

    const char* TAG = "spi_loop";
    static int counter = 0;

    ESP_LOGI(TAG, "Loop: counter=%d", ++counter);

    out.user((void*) 1);    // D/C command mode
    out.sputc(0x04);

    //in.sbumpc();
    uint32_t lcd_id = 0;
    in.sgetn((char*)&lcd_id, 3);

    ESP_LOGI(TAG, "LCD ID =%d", lcd_id);

    estd::this_thread::sleep_for(delay);
}

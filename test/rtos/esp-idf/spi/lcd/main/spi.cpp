#include <estd/chrono.h>
#include <estd/thread.h>

//#include <chrono>

#include <embr/platform/esp-idf/spi.h>

#include "spi.h"

#include <esp_log.h>
#include <driver/gpio.h>

#include <esp_lcd_panel_commands.h>

using namespace embr::esp_idf;

//This function is called (in irq context!) just before a transmission starts. It will
//set the D/C line to the value indicated in the user field.
void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    const char* TAG = "lcd_spi_pre_transfer_callback";

    int dc=(int)t->user;

    //ESP_LOGV(TAG, "dc=%d", dc);

    gpio_set_level((gpio_num_t)PIN_NUM_DC, dc);
}

static spi::bus bus(LCD_HOST);
static spi::device device;

void lcd_init(spi::device device);

static void exp1()
{
    // This is known to be pretty large since it houses a circular queue of transactions
    static test_interrupt_ostreambuf test(device);

    // Both compile - need a different way to test though this will confuse the ILI9341
    // Still needs lots of work, including listening to post callback to pop queue... hopefully
    // it doesn't happen out of order!!
    //test.sputc('h');
    //test.sputn("ello", 4);

    /*
    spi_master_ostreambuf out(device);
    out.user((void*) 0);    // D/C command mode

    out.sputc(LCD_CMD_DISPON);
    out.sputc(LCD_CMD_SLPOUT); */
}

// There's a distant chance the compiler bug which throws these initializer warnings may be leaving
// our underlying data unzeroed.  So doing that here.  Only a 5% chance, but given all the pain experienced
// today I'm not taking any chances.  For the record, makes no difference that I can see on run
void spi_fixup_exp(spi_bus_config_t& buscfg)
{
    buscfg.flags = 0;
    buscfg.data4_io_num = 0;
    buscfg.data5_io_num = 0;
    buscfg.data6_io_num = 0;
    buscfg.data7_io_num = 0;
}


void spi_fixup_exp(spi_device_interface_config_t& devcfg)
{
    devcfg.command_bits = 0;
    devcfg.address_bits = 0;
    devcfg.dummy_bits = 0;
    devcfg.duty_cycle_pos = 0;
    devcfg.cs_ena_pretrans = 0;
    devcfg.cs_ena_posttrans = 0;
    devcfg.input_delay_ns = 0;
    devcfg.flags = 0;
    devcfg.post_cb = nullptr;
}

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

    spi_fixup_exp(buscfg);

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

    spi_fixup_exp(devcfg);

    //Initialize the SPI bus
    ret=bus.initialize(buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    device = bus.add(devcfg);

    // DEBT: Clunky, but will get us through the short term OK
    //ESP_ERROR_CHECK(device.add(bus, devcfg));

    lcd_init(device);
    exp1();
}

void spi_loop()
{
    spi_master_ostreambuf out(device);
    spi_master_istreambuf in(device);

    // Surprisingly, these are not available
    //using namespace std::chrono_literals;

    // This one is not ready yet
    //using namespace estd::literals::chrono_literals;
    constexpr estd::chrono::milliseconds delay(1000);

    const char* TAG = "spi_loop";
    static int counter = 0;
    uint8_t brightness = counter % 16 * 0xF;
    uint8_t brightness_in = 0;

    out.user((void*) 0);    // D/C command mode
    in.user((void*) 1);     // D/C data mode (separate input streambuf is pretty much always in this mode)

    /*
    out.sputc(LCD_CMD_RDDISBV);
    brightness_in = in.sbumpc(); */

    ESP_LOGI(TAG, "Loop: counter=%d, brightness=%d, brightness_in=%d", ++counter, brightness, brightness_in);

    out.sputc(0x04);    // ID request
    out.pubsync();

    /*
    out.sputc(0x54);    // Read CTRL Display
    brightness_in = in.sbumpc();
    ESP_LOGD(TAG, "Read CTRL Display: %08X", brightness_in);

    out.sputc(LCD_CMD_WRDISBV);
    out.user((void*) 1);    // D/C data mode
    out.sputc(brightness);
    out.pubsync(); */

    if(counter % 2 == 0)
    {
        uint32_t lcd_id = 0xFFFF;
        in.sgetn((char*)&lcd_id, 3);
        // NOTE: We may get fancy at some point and lcd_id might not actually get populated until
        // sync finishes
        in.pubsync();

        out.sputc(LCD_CMD_DISPOFF);
        //out.sputc(LCD_CMD_SLPIN);

        ESP_LOGI(TAG, "LCD ID=%08X - sgetn", lcd_id);
    }
    else
    {
        unsigned lcd_id = 0;

        // DEBT: Not sure if this really is gonna work right due to endianness and our ILI reports '0' anyway
        lcd_id = in.sbumpc();
        lcd_id <<= 8;
        lcd_id |= in.sbumpc();
        lcd_id <<= 8;
        lcd_id |= in.sbumpc();

        out.sputc(LCD_CMD_DISPON);
        //out.sputc(LCD_CMD_SLPOUT);

        ESP_LOGI(TAG, "LCD ID=%08X - sbumpc", lcd_id);
    }

    estd::this_thread::sleep_for(delay);
}

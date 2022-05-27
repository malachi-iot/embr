// Switch away from the LCD defaults and use our own CS line

#undef PIN_NUM_CS
#define PIN_NUM_CS 4

void copy_to_buscfg(spi_bus_config_t* copy_to)
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

    memcpy(copy_to, &buscfg, sizeof(buscfg));
}


void copy_to_devcfg(spi_device_interface_config_t* copy_to)
{
    spi_device_interface_config_t devcfg={
        .mode=0,                                //SPI mode 0
        .clock_speed_hz=1*10*1000,              //Clock out at 1 MHz
        .spics_io_num=PIN_NUM_CS,               //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time

        // bookend
        .post_cb=NULL
        //.post_cb = test_interrupt_ostreambuf::post_cb
    };

    memcpy(copy_to, &devcfg, sizeof(devcfg));
}

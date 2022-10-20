#include <driver/gpio.h>

#include "gpio.h"

void gpio_init()
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    //disable pull-down mode
    io_conf.pull_down_en = (gpio_pulldown_t)0;
    //disable pull-up mode
    io_conf.pull_up_en = (gpio_pullup_t)0;
    //io_conf.pull_up_en = (pull_mode == GPIO_PULLUP_ONLY || pull_mode == GPIO_PULLUP_PULLDOWN);
    //io_conf.pull_down_en = (pull_mode == GPIO_PULLDOWN_ONLY || pull_mode == GPIO_PULLUP_PULLDOWN);;
    //io_conf.pin_bit_mask = pin_select;
    gpio_config(&io_conf);
}
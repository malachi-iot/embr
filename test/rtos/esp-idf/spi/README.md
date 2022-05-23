# SPI test

We wrap SPI API with a streambuf

Guidance from 

https://github.com/espressif/esp-idf/blob/v4.4.1/examples/peripherals/spi_master/lcd/main/spi_master_example_main.c

## Prerequisites

At the moment, an ILI 9341 hooked up exactly how a ESP32-WROVER-KIT
does it is necessary

## Results

| Date    | esp-idf | Platform | Result |
|---------| ------- | -------- | ------ |
| 23MAY21 | v4.4.1  | ESP32-WROVER-KIT E | Pass* |

Asterisk because it's unclear whether we *really* got the
right data back, since ILI9341 always reports a zero here
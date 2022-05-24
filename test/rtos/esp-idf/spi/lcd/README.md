# SPI test

We wrap SPI API with a streambuf

Guidance from 

https://github.com/espressif/esp-idf/blob/v4.4.1/examples/peripherals/spi_master/lcd/main/spi_master_example_main.c

## Prerequisites

At the moment, an ILI 9341 hooked up exactly how a ESP32-WROVER-KIT
does it is necessary

## Results

| Date    | esp-idf | Platform           | Result    |
|---------|---------|--------------------|-----------|
| 23MAY21 | v4.4.1  | ESP32-WROVER-KIT E | Unknown   |


### Thoughts

It looks like we're failing.  The status bytes we read back are 0xFF, and indeed after our init phase
we read infinite 0xFF's until something mysterious pops us out into reading infinite 0x00's.

No errors are reported from `esp-idf` API, and it even indicates we're getting real characters back.

I've clocked about 4 hours trying to diagnose this one behavior so far.

Others experiencing similar issues:

* https://www.esp32.com/viewtopic.php?t=21347
* https://esp32.com/viewtopic.php?t=14277
* https://github.com/espressif/esp-idf/issues/7141 
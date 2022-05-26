# Overview

The way of doing SPI slave, even with Arduino's help, is vastly
different between AVR and SAMD.  Therefore, making this helper tool
alongside slave-avr flavor

## Detail

Using https://github.com/lenvm/SercomSPISlave library

Ran into an issue using Sercom0 (compiler complained method was already
declared, maybe for Serial UART?).  So used Sercom1, which compiles great.
Excerpt from above link, mapped to pinouts for Feather M0
As per [here](https://learn.adafruit.com/adafruit-feather-m0-basic-proto/pinouts) and [here](
https://cdn-learn.adafruit.com/assets/assets/000/046/244/original/adafruit_products_Feather_M0_Basic_Proto_v2.2-1.png?1504885373).
mappings are:

Sercom1:

| SPI  | Native | Arduino |
| ---- | ------ | ------- |
| MOSI | PA16   | D11     |
| SCK  | PA17   | D13     |
| SS   | PA18   | D10     |
| MISO | PA19   | D12     |



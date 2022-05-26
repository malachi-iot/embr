# Slave support device

Hitting so many issues with ILI9341 + ESP32 that I have to roll a simplified slave SPI device.
That's what this is

Comes from

https://gist.github.com/chrismeyersfsu/3317769

## Part Used - ProMicro

My board looks exactly like this:

![Cheap ProMicro from eBay](https://preview.redd.it/g33ymjfjwf041.png?width=1214&format=png&auto=webp&s=142e15a1d58556e00fb6fbedc2a4fdd5292f63e9)

Also, as an aside, this image implies we might be able to reconfigure operating voltage?  I think the 32u4 has to be a different part though.

![Pay attention to J1](https://golem.hu/pic/pro_micro_pinout.jpg)

Anyway, from these two we deduce pinouts to be used are:

|  SPI  |  Arduino Pin | 
| ----- | ------------ |
| MOSI  | D16          |
| SCLK  | D15          |
| MISO  | D14          |

Apparently, we cannot use ProMicro due to lack of an exposed SS pin

https://forum.arduino.cc/t/sparkfun-pro-micro-as-spi-slave-problems/158291/4 
https://arduino.stackexchange.com/questions/16348/how-do-you-use-spi-on-an-arduino

The hack is literally a hack, you have to cut the trace to the LED and wire
directly to the AVR.  Ugh.
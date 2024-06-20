# Overview

# 1.

# 3. Opinions & Observations

## 3.1. No Documentation

I can't seem to find any good documentation anywhere for TinyUSB.

## 3.2. Freshness

Espressif's fork [1.1] appears kinda old and their examples seem to rely a lot
on their own layer [1.2]

## 3.3. Espressif's layer

Due to 3.1., will have to poke around to see what lives where.  Here are some calls
identified as Espressif specific:

* `tinyusb_cdcacm_write_flush` [1.2]

# 4. 

# References

1. https://github.com/hathach/tinyusb
    1. https://github.com/espressif/tinyusb
    2. https://github.com/espressif/esp-usb
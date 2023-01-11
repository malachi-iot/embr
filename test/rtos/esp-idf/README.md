# Overview

Requires `esp-idf` v4.x

I think the CMakeLists.txt in this same folder is not used
## test code

### button

### observer


### scheduler

### unity

Watchdog likes to be off for unit tests I guess they block a lot

## Results

These tests are all for variants of Espressif ESP32

|   Date  | Project      | Board                | Chip           | esp-idf  | Result  | Notes
| ------- | ------------ | -------------------- | -------------- | -------  | ------- | -----
| 14DEC22 | button       | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass    | FreeRTOS 'held' works at a rudimentary level
| 03JAN23 | button       | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v5.0.0   | Pass    |
| 04JAN23 | button       | ESP32-C3-DevKitM-1   | ESP32C3        | v5.0.0   | Pass    |
| 19DEC22 | observer     | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass    |
| 15DEC22 | scheduler    | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass    |
| 30DEC22 | scheduler    | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v5.0.0   | Fail    | Minor SDK incompatibilities inhibit compilation
| 30DEC22 | udp-echo     | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass    |
| 30DEC22 | udp-echo     | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v5.0.0   | Pass    |
| 30DEC22 | unity        | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass    |
| 03JAN23 | unity        | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v5.0.0   | Pass    |
| 10JAN23 | unity        | ESP32-C3-DevKitM-1   | ESP32C3        | v5.0.0   | Fail    | LWIP_TCPIP_CORE_LOCKING glitch [1]

# References

1. https://github.com/espressif/esp-idf/issues/10466 



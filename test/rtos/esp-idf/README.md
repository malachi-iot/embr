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
| 07DEC22 | button       | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass    | Long-press experimental FreeRTOS code fails, but is not yet part of this test
| 07DEC22 | button       | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v5.0.0   | Pass    | ^^^
| 04DEC22 | scheduler    | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass    |
| 04DEC22 | udp-echo     | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass    |
| 14DEC22 | unity        | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass    |
| 07DEC22 | unity        | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v5.0.0   | Pass    |



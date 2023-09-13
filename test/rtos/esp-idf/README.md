# Overview

Requires `esp-idf` v4.4.4 or higher

I think the CMakeLists.txt in this same folder is not used

## c++ findings

Noteworthy is that esp-idf < 5.0 = c++11, >= 5.0 = c++20
Also, despite this, syntax #8 from https://en.cppreference.com/w/cpp/language/namespace works in < 5.0

## test code

### button

### observer

### scheduler

### services

### unity

Watchdog likes to be off for unit tests I guess they block a lot

## Results

These tests are all for variants of Espressif ESP32

|   Date  | Project      | Board                | Chip           | esp-idf  | Result  | Notes
| ------- | ------------ | -------------------- | -------------- | -------  | ------- | -----
| 14DEC22 | button       | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass     | FreeRTOS 'held' works at a rudimentary level
| 03JAN23 | button       | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v5.0.0   | Pass     |
| 04JAN23 | button       | ESP32-C3-DevKitM-1   | ESP32C3        | v5.0.0   | Pass     |
| 15AUG23 | debounce     | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v5.0.3   | Pass     |
| 18AUG23 | debounce     | ESP32C3 Xiao         | ESP32C3        | v5.1     | Pass     | Tested against GPIO9 boot button
| 19DEC22 | observer     | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass     |
| 15AUG23 | observer     | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v5.0.3   | Pass     |
| 15DEC22 | scheduler    | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass     |
| 15AUG23 | scheduler    | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v5.0.3   | Pass     |
| 14JAN23 | scheduler    | WEMOS D1             | ESP32          | v5.0     | Pass     |
| 14JAN23 | scheduler    | WEMOS D1             | ESP32          | v5.1-dev | Fail     | Runs, wake is broken - smells like a race condition
| 03AUG23 | services/gpio | ESP-WROVER-KIT v4.1 | ESP32-WROVER-E | v5.0.3   | Pass     |
| 03SEP23 | services/gpio | ESP32C3 Xiao        | ESP32C3        | v5.1.1   | Pass     |
| 15AUG23 | services/gptimer | ESP-WROVER-KIT v4.1 | ESP32-WROVER-E | v5.0.3   | Compiles     |
| 25AUG23 | services/gptimer | FeatherS3        | ESP32S3        | v5.0.3   | Partial  | Runs, but 2nd time it goes to sleep it never wakes up
| 28AUG23 | services/gptimer | ESP32C3 Xiao     | ESP32C3        | v5.1.1   | Pass     |
| 28AUG23 | services/ledc | ESP32-C3-DevKitM-1  | ESP32C3        | v5.1.1   | Pass     | 
| 03AUG23 | services/twai | ESP-WROVER-KIT v4.1 | ESP32-WROVER-E | v5.0.3   | Pass     |
| 03SEP23 | services/twai | ESP32C3 Xiao        | ESP32C3        | v5.1.1   | Pass     |
| 17AUG23 | services/wifi | ESP32C3 Xiao        | ESP32C3        | v5.1     | Pass     |
| 11JAN23 | timer-sched  | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v5.0.0   | Pass     | Runtime complaints about timer not initializing - still runs
| 30DEC22 | udp-echo     | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass     |
| 30DEC22 | udp-echo     | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v5.0.0   | Pass     |
| 30DEC22 | unity        | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass     |
| 15AUG23 | unity        | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v5.0.3   | Pass | Was hitting LWIP_TCPIP_CORE_LOCKING glitch[^1], not now
| 13SEP23 | unity        | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v5.1.1   | Pass     |
| 10JAN23 | unity        | ESP32-C3-DevKitM-1   | ESP32C3        | v5.0.0   | Pass     | 
| 19AUG23 | unity        | ESP32-C3-DevKitM-1   | ESP32C3        | v5.1     | Pass     | 
| 10JUL23 | unity        | WEMOS D1             | ESP32          | v5.0.2   | Pass     |
| 25AUG23 | unity        | FeatherS3            | ESP32S3        | v5.1.1   | Pass     | 

# References

[^1]: https://github.com/espressif/esp-idf/issues/10466 



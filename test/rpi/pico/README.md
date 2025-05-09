# Raspberry Pi Pico examples

## CMake

Copy `wifi.cmake.template` to `wifi.cmake` to specify credentials

Remember to specify `-DCMAKE_BUILD_TYPE=Debug` if you want to see LwIP debug output

### sdk caveats

As per [6] there are some unknowns/caveats to the particular placement of that
include and subsequent `pico_sdk_init`.  For the time being, it seems that
include MAY precede `project` and it is verified that `pico_sdk_init` SHOULD
be called after `project` (warnings from SDK indicate this) - which is in 
contrast to recommendation from [6]

SDK warning in particular:

```
CMake Warning at /home/malachi/Projects/ext/rpi/pico-sdk/pico_sdk_init.cmake:53 (message):
  pico_sdk_init() should be called after the project is created (and
  languages added)
```

## Journal

### 09MAY25

Scrubbing build in support of removing ext/estdlib.  However, nothing I do results in a successful compile.  In particular:

```
/home/malachi/Projects/iot/embr/test/rpi/pico/support/init.cpp:4:10: fatal error: pico/cyw43_arch.h: No such file or directory
    4 | #include <pico/cyw43_arch.h>
      |          ^~~~~~~~~~~~~~~~~~~
```

Hits us every time.  I've spent an hour on that, trying:

- Linking in pico_cyw43_arch_none, pico_cyw43_driver
- Diddling with ways in which `pico_sdk_init.cmake` is pulled in
- Extra effort in subodule retrieval for SDK itself

I give up.  I'm not using RPI Pico at the moment anyway

## Projects

### udp-echo

Utilizes `udpecho.py` associated with esp32 udp-echo test

### unity

### support

LwIP + USB specific setup

## Results

|   Date  | Project      | Board                | Chip           | pico-sdk | Result  | Notes
| ------- | ------------ | -------------------- | -------------- | -------- | ------- | -----
| 26DEC22 | unity        | Raspberry Pi Pico W  | RP2040         | v1.4.0   | Fail    | Almost works, but one of the LwIP test fails
| 27DEC22 | udp-echo     | Raspberry Pi Pico W  | RP2040         | v1.4.0   | Pass    | Works in background+polled modes

# References

1. RESERVED for example code
2. https://www.cnx-software.com/2022/07/03/getting-started-with-wifi-on-raspberry-pi-pico-w-board/
3. https://forums.raspberrypi.com/viewtopic.php?t=337666
4. https://www.i-programmer.info/programming/hardware/15838-the-picow-in-c-simple-web-client.html?start=2
5. https://forums.raspberrypi.com/viewtopic.php?t=309441
5. https://github.com/raspberrypi/pico-sdk
   1. https://github.com/raspberrypi/pico-sdk/blob/1.4.0/src/board_setup.cmake
6. https://admantium.medium.com/getting-started-with-raspberry-pico-and-cmake-f536e18512e6
7. https://raspberrypi.github.io/pico-sdk-doxygen/group__pico__cyw43__arch.html


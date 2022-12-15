# Raspberry Pi Pico examples

## CMake

Copy `wifi.cmake.template` to `wifi.cmake` to specify credentials

Remember to specify `-DCMAKE_BUILD_TYPE=Debug` if you want to see LwIP debug output

## Project

### udp-echo

Utilizes `udpecho.py` associated with esp32 udp-echo test

### unity

## Results

|   Date  | Project      | Board                | Chip           | pico-sdk | Result  | Notes
| ------- | ------------ | -------------------- | -------------- | -------- | ------- | -----
| 14DEC22 | unity        | Raspberry Pi Pico W  | RP2040         | v1.4.0   | Fail    | Almost works, but one of the LwIP test fails
| 11DEC22 | udp-echo     | Raspberry Pi Pico W  | RP2040         | v1.4.0   | Pass    | Works in background+polled modes

# References

1. RESERVED for example code
2. https://www.cnx-software.com/2022/07/03/getting-started-with-wifi-on-raspberry-pi-pico-w-board/
3. https://forums.raspberrypi.com/viewtopic.php?t=337666
4. https://www.i-programmer.info/programming/hardware/15838-the-picow-in-c-simple-web-client.html?start=2
5. https://forums.raspberrypi.com/viewtopic.php?t=309441
5. https://github.com/raspberrypi/pico-sdk
   1. https://github.com/raspberrypi/pico-sdk/blob/1.4.0/src/board_setup.cmake
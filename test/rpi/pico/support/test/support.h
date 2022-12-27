#pragma once

#include <estd/ostream.h>

namespace estd {

extern pico_ostream clog;

}

namespace test {
    
namespace v1 {

// inits USB output and wifi hardware, though makes no connection attempt
int init();

// inits USB output and makes wifi connection attempt
int init(const char* ssid, const char* wifi_password);

// executes cyw43 arch poll to do lwip and wifi housekeeping, then delays by
// 1ms, then calls lwip_poll
void cyw43_poll();

// checks to see if interface is connected and has an IP and when that happens,
// clog's it
void lwip_poll();

// Prints a debug counter and sleeps for 5s
// NOTE: Only for use in background/non polled mode
void sleep();


}

}

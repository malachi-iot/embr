#pragma once

#include <estd/chrono.h>

#include <esp_system.h>
#include <soc/rtc.h>
#include <xtensa/core-macros.h>

// As per:
// https://esp32.com/viewtopic.php?t=16228
// https://github.com/espressif/esp-idf/blob/master/components/xtensa/include/xtensa/core-macros.h 
// EXPERIMENTAL
struct xthal_clock
{
    typedef uint32_t rep;
    typedef estd::ratio<1, 80000000> period;
    typedef estd::chrono::duration<rep, period> duration;
    typedef estd::chrono::time_point<xthal_clock> time_point;

    static CONSTEXPR bool is_steady = false;

    static time_point now()
    {
        rep raw = XTHAL_GET_CCOUNT();
        /*
        rtc_cpu_freq_t f = rtc_clk_cpu_freq_get();

        switch(f)
        {
            case rtc_cpu_freq_t::RTC_CPU_FREQ_80M:
                break;

            case rtc_cpu_freq_t::RTC_CPU_FREQ_160M:
                raw /= 2;
                break;

            case rtc_cpu_freq_t::RTC_CPU_FREQ_240M:
                raw /= 3;
                break;
        }
        */

        return time_point(duration(raw));
    }
};
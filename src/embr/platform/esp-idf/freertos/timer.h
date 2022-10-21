#pragma once

#pragma once

#if ESP_PLATFORM
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#else
#include <timers.h>
#endif

// DEBT: Move into estd
namespace embr { namespace freertos { namespace wrapper {

class timer
{
    TimerHandle_t h;

public:
}

}}}
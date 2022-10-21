#pragma once

#pragma once

#if ESP_PLATFORM
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#else
#include <timers.h>
#endif

// DEBT: Move into estd
namespace embr { namespace freertos { 
    
namespace wrapper {

class timer
{
    TimerHandle_t h;

public:
    timer(TimerHandle_t h) : h(h) {}

    static timer create(const char* const pcTimerName,
        const TickType_t xTimerPeriod,
        const UBaseType_t uxAutoReload,
        void* const pvTimerID,
        TimerCallbackFunction_t pxCallbackFunction)
    {
        return xTimerCreate(
            pcTimerName, xTimerPeriod,
            uxAutoReload, pvTimerID,
            pxCallbackFunction);
    }


    static timer create(const char* const pcTimerName,
        const TickType_t xTimerPeriod,
        const UBaseType_t uxAutoReload,
        void* const pvTimerID,
        TimerCallbackFunction_t pxCallbackFunction,
        StaticTimer_t* pxTimerBuffer)
    {
        return xTimerCreateStatic(
            pcTimerName, xTimerPeriod,
            uxAutoReload, pvTimerID,
            pxCallbackFunction,
            pxTimerBuffer);
    }

    BaseType_t start(TickType_t xBlockTime)
    {
        return xTimerStart(h, xBlockTime);
    }


    BaseType_t stop(TickType_t xBlockTime)
    {
        return xTimerStop(h, xBlockTime);
    }

    void set_timer_id(void* pvNewId)
    {
        vTimerSetTimerID(h, pvNewId);
    }

    void* get_timer_id() const
    {
        return pvTimerGetTimerID(h);
    }
};

}

template <bool is_static = false>
class timer;

}}
#pragma once

#include <estd/port/freertos/mutex.h>
#if ESP_PLATFORM
#include "../esp-idf/mutex.h"
#endif

namespace embr { namespace freertos {

// Unlike estd::freertos things, we make no effort to conform to std here.  Instead,
// we have more specific and preconfigured mutexes here to feed into thunk, msg-bipbuf
template <unsigned ms, bool static_allocated = true>
struct timed_mutex
{
    estd::freertos::timed_mutex<static_allocated> mutex_;

    bool lock()
    {
        return mutex_.try_lock_for(estd::chrono::milliseconds(ms));
    }

    void unlock() { mutex_.unlock(); }
};

#if ESP_PLATFORM
using hw_mutex = esp_idf::hw_mutex;
using hw_mutex_isr = esp_idf::hw_mutex_isr;
#else
// UNTESTED
struct hw_mutex
{
    bool lock()
    {
        taskENTER_CRITICAL();
        return true;
    }

    void unlock()
    {
        taskEXIT_CRITICAL();
    }
};
#endif

}}
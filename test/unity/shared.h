#pragma once

#include <estd/port/freertos/semaphore.h>
#include <estd/port/freertos/thread.h>

namespace embr { namespace test {

// Task notifications are great, but if I don't get them exactly right they goof up
// other unit tests it seems. 
#define USE_TASK_NOTIFICATION_SHARED 0

// For worker task scenarios
struct shared
{
    using semaphore = estd::freertos::wrapper::semaphore;

#if USE_TASK_NOTIFICATION_SHARED
    constexpr int notification_index = 0;

    rtos::task parent = rtos::task::current();
#else
    static semaphore finished;
#endif

    void finish()
    {
#if USE_TASK_NOTIFICATION_SHARED
        parent.notify_give(notification_index);
#else
        finished.give();
#endif        
    }

    // DEBT: Sure feels like FreeRTOS would have something like this already
    static void wait(int task_count)
    {
        for(int i = 0; i < task_count; ++i)
        {
#if USE_TASK_NOTIFICATION_SHARED
            ulTaskNotifyTakeIndexed(notification_index, pdFALSE, portMAX_DELAY);
#else
            finished.take(portMAX_DELAY);
#endif
    }
    }
};


}}
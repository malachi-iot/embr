#include <estd/chrono.h>
#include <estd/thread.h>

#include "scheduler.h"

using namespace estd::chrono;
using namespace estd::literals;

#if SCHEDULER_APPROACH == SCHEDULER_APPROACH_TASKNOTIFY
void scheduler_daemon_task(void*)
{
//    typedef FunctorTraits::time_point time_point;

    for(;;)
    {
        auto duration = scheduler.top_time() - freertos_clock::now();
        ulTaskNotifyTake(pdFALSE, duration.count());

        scheduler.process();
    }
}
#endif
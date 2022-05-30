#include <estd/chrono.h>
#include <estd/thread.h>

#include "scheduler.h"

using namespace estd::chrono;
using namespace estd::literals;

#if SCHEDULER_APPROACH == SCHEDULER_APPROACH_TASKNOTIFY
void scheduler_daemon_task(void*)
{
    for(;;)
    {
        estd::this_thread::sleep_for(milliseconds(100));

        scheduler.process();
    }
}
#endif
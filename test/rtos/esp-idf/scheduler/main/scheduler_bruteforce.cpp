#include "scheduler.h"

#if SCHEDULER_APPROACH == SCHEDULER_APPROACH_BRUTEFORCE
void scheduler_daemon_task(void*)
{
    for(;;)
    {
        scheduler.process();

        vTaskDelay(1);
    }
}
#endif
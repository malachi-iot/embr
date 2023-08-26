#include <esp_log.h>

#include "worker.h"

namespace embr { namespace freertos { namespace worker {

// TODO: Try using xTimerPendFunctionCall and re-use timer service task instead of making
// our own worker.  The thing is, portMAX_DELAY (and timeout related functions) tend to not
// work well when executed FROM timer task.  Perhaps dequeue_from_isr works better?
#if FEATURE_TMR_WORKER
inline void pend_service(void * pvParameter1, uint32_t ulParameter2)
{
    queue.dequeue(0);
}
#else
#endif


inline void Service::worker(void* arg)
{
    ESP_LOGD(TAG, "worker: dedicated task entry");

    auto s = (Service*) arg;
    
    for(;;)
    {
        s->queue.dequeue(portMAX_DELAY);
    }
}

inline Service::state_result Service::on_start()
{
    // DEBT
    constexpr unsigned CONFIG_MIOT_WORKER_TASK_STACK = 4096;

#if FEATURE_TMR_WORKER == 0
    // It's helpful to run this at a not-the-lowest priority so that it can
    // edge out timer task etc
    BaseType_t ret = xTaskCreate(worker, TAG, CONFIG_MIOT_WORKER_TASK_STACK, this, 4, NULL);

    if(ret != pdTRUE)
    {
        return state_result{Error, ErrMemory};
    }

#endif

    // DEBT: Set started state in worker area itself
    return state_result::started();
}

}}}


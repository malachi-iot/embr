#pragma once

#include <embr/internal/delegate_queue.h>
#include <embr/service.h>

// Doing worker thread stuff on timer thread works quite well.  That said,
// the timer thread runs at a lower priority which means the worker thread
// goes a lot slower when you run it on the timer.  You could also change
// configTIMER_TASK_PRIORITY to elevate all timer tasks in general too.
#define FEATURE_TMR_WORKER 0


#if INCLUDE_xTimerPendFunctionCall && configUSE_TIMERS
#ifndef FEATURE_TMR_WORKER
#define FEATURE_TMR_WORKER 1
#endif
#if FEATURE_TMR_WORKER
#include <estd/port/freertos/timer.h>
#endif
#else
#undef FEATURE_TMR_WORKER
#endif

namespace embr { namespace freertos { namespace worker { inline namespace v1 {

typedef embr::internal::delegate_queue<> queue_type;

// DEBT: Pass in particular queue of interest to pend service parameter
#if FEATURE_TMR_WORKER
void pend_service(void * pvParameter1, uint32_t ulParameter2);
inline void pend_service()
{
    xTimerPendFunctionCall(pend_service, NULL, 0, portMAX_DELAY);
}
#else
constexpr void pend_service() {}
#endif

template <typename F>
inline void invoke(queue_type& queue, F&& f)
{
    queue.enqueue(std::move(f), portMAX_DELAY);
    pend_service();
}

template <typename F>
inline void invoke_from_isr(queue_type& queue, F&& f)
{
    queue.enqueue_from_isr(std::move(f), nullptr);
    pend_service();
}


struct Service : embr::Service
{
    typedef Service this_type;

    static constexpr const char* TAG = "Worker";
    static constexpr const char* name() { return TAG; }

    queue_type queue;

    EMBR_PROPERTY_RUNTIME_BEGIN(embr::Service)

    EMBR_PROPERTY_RUNTIME_END

    static void worker(void*);

    state_result on_start();

    Service(size_t sz) : queue(sz) {}
};


extern Service& queue;


}}

}}

namespace embr { namespace freertos { namespace service { inline namespace v1 {

using Worker = embr::freertos::worker::v1::Service;

}}}}

template <typename F>
inline embr::freertos::worker::queue_type& operator <<(
    embr::freertos::worker::queue_type& wq, F&& f)
{
    wq.enqueue(std::move(f), portMAX_DELAY);
    embr::freertos::worker::pend_service();
    return wq;
} 


#if FEATURE_TMR_WORKER == 0
template <typename F>
inline embr::freertos::worker::queue_type& operator <<(
    embr::freertos::worker::Service& service, F&& f)
{
    service.queue.enqueue(std::move(f), portMAX_DELAY);
    //worker::pend_service();
    return service.queue;
}
#endif


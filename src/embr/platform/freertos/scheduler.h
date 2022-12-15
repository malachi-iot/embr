#pragma once

#include <estd/chrono.h>
#include <estd/thread.h>    // Brings in FreeRTOS task notify, BaseType_t and friends

#include <estd/port/freertos/mutex.h>

// DEBT: For SchedulerContext, but feels like context_factory should be
// elsewhere entirely
#include "../../internal/impl/scheduler.h"

#ifdef ESP_PLATFORM
#include "esp_log.h"
#endif

namespace embr { namespace scheduler { namespace freertos {

// DEBT: This works well, but it would be nice for context to have clearer const
// and non const separation
// DEBT: Namespace likely needs work - should be in an 'impl' and/or 'context' namespace
struct context_type
{
    // For mutex specifically
    // DEBT: Might need better name
    enum TimeoutModes
    {
        Blocking,       ///< Blocks until mutex is acquired

        // Silent and Warning not useful for scheduler activities because for the time being
        // they are do or die.  In a broader mutex-wrapper kind of a way
        // (decoupled from scheduler), this might be useful
        //Silent,         ///< Times out and records result in context without emitting a warning
        //Warning,      // TBD: Same as above and Emits a warning log on timeout

        Abort           ///< Times out and signals an abort program if mutex is not acquired
    };

    union
    {
        // to catch output from xSemaphoreGiveFromISR and friends
        BaseType_t higherPriorityTaskWoken;
        // output code from timed lock operation
        BaseType_t timedLockResult;
    };

    // DEBT: Make this configurable
    static constexpr estd::chrono::milliseconds mutex_timeout()
    {
        return estd::chrono::milliseconds(50);
    }

    static constexpr TimeoutModes mutex_timeout_mode()
    {
        return Abort;
    }
};

struct context_factory
{
    template <class TScheduler, class TUserContext>
    inline static embr::internal::SchedulerContext<TScheduler, TUserContext> create_context(
        TScheduler& scheduler, TUserContext& user_context, bool in_isr, bool use_mutex = true)
    {
#if defined(ESP_PLATFORM)
        const char* TAG = "context_factory (FreeRTOS flavor)";

        ESP_DRAM_LOGV(TAG, "create_context: entry - in_isr=%u", in_isr);
#endif
        embr::internal::SchedulerContext<TScheduler, TUserContext> context(scheduler, user_context, in_isr, use_mutex);

        context.higherPriorityTaskWoken = false;

        return context;
    }
};



// Special timed_mutex wrapper built for scheduler usage
template <bool binary_semaphore, bool is_static>
struct timed_mutex : estd::freertos::timed_mutex<is_static>
{
    typedef estd::freertos::timed_mutex<is_static> base_type;

    // DEBT: Not 100% convinved binary-sempahore-mutex is the way to go
    timed_mutex() : base_type(binary_semaphore)
    {
        if(binary_semaphore)
        {
            // NOTE: This can only run from non ISR
            // DEBT: Clumsy, but seems to work
            base_type::unlock();
        }
    }

protected:
    template <class TScheduler>
    inline void lock_in_isr(embr::internal::SchedulerContextBase<TScheduler>& context)
    {
        base_type::native_handle().take_from_isr(&context.higherPriorityTaskWoken);
    }


    template <class TScheduler>
    inline void lock_non_isr(embr::internal::SchedulerContextBase<TScheduler>& context)
    {
        switch(context.mutex_timeout_mode())
        {
            default:
            case context_type::Blocking:
                base_type::lock();
                break;

            //case Silent:
                //context.timedLockResult = base_type::try_lock_for(context.mutex_timeout());
                //break;

            case context_type::Abort:
                assert(base_type::try_lock_for(context.mutex_timeout()));
                break;
        }
    }


    template <class TScheduler>
    inline void unlock_non_isr(embr::internal::SchedulerContextBase<TScheduler>& context)
    {
        base_type::unlock();
    }


public:

    template <class TScheduler>
    inline void lock(embr::internal::SchedulerContextBase<TScheduler>& context)
    {
        if(context.in_isr())
        {
            lock_in_isr(context);
        }
        else
        {
            lock_non_isr(context);
        }
    }

    template <class TScheduler>
    inline void unlock(embr::internal::SchedulerContextBase<TScheduler>& context)
    {
        if(context.in_isr())
        {
            base_type::native_handle().give_from_isr(&context.higherPriorityTaskWoken);
        }
        else
            unlock_non_isr(context);
    }
};


}}}
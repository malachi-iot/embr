#pragma once

#include <estd/chrono.h>
#include <estd/port/freertos/mutex.h>
#include <estd/port/freertos/thread.h>

#include <embr/scheduler.hpp>

#ifdef ESP_PLATFORM
#include "esp_log.h"
#endif

namespace embr { namespace scheduler { namespace freertos {

template <class TScheduler>
void notify_daemon_task(void* data);


}}}

namespace embr { namespace freertos { namespace experimental {

/// Dummy/no-op observer
/// @tparam TTraits
template <class TImpl>
struct SchedulerObserver
{
    typedef TImpl traits_type;

    void on_notify(embr::internal::events::Processing<traits_type>)
    {
    }

    void on_notify(embr::internal::events::Scheduling<traits_type>)
    {
    }

    void on_notify(embr::internal::events::Scheduled<traits_type>)
    {
    }

    void on_notify(embr::internal::events::Removed<traits_type>)
    {
    }
};


// DEBT: Break this out a bit the way esp32 timer mode was where we pass in control_structure
// but definitely lean heavily towards *default* being FunctorImpl - so that we can do FreeRTOS
// stuff without: 1) being bound to functors and 2) using simpler on_scheduled vs. subject/observer pattern
struct FunctorImpl :
    embr::internal::scheduler::impl::Function<estd::chrono::freertos_clock::time_point>
{
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

    template <bool binary_semaphore>
    // 'true' designates static allocation
    struct timed_mutex : estd::freertos::timed_mutex<true>
    {
        typedef estd::freertos::timed_mutex<true> base_type;

        // DEBT: Not 100% convinved binary-sempahore-mutex is the way to go
        timed_mutex() : base_type(binary_semaphore) {}

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

        template <class TScheduler>
        inline void lock(embr::internal::SchedulerContextBase<TScheduler>& context)
        {
            if(context.in_isr())
            {
                native_handle().take_from_isr(&context.higherPriorityTaskWoken);
            }
            else
            {
                switch(context.mutex_timeout_mode())
                {
                    default:
                    case Blocking:
                        base_type::lock();
                        break;

                    //case Silent:
                        //context.timedLockResult = base_type::try_lock_for(context.mutex_timeout());
                        //break;

                    case Abort:
                        assert(base_type::try_lock_for(context.mutex_timeout()));
                        break;
                }
            }
        }

        template <class TScheduler>
        inline void unlock(embr::internal::SchedulerContextBase<TScheduler>& context)
        {
            if(context.in_isr())
            {
                native_handle().give_from_isr(&context.higherPriorityTaskWoken);
            }
            else
                base_type::unlock();
        }
    };

    typedef timed_mutex<false> mutex;
    typedef timed_mutex<true> binary_semaphore;

    inline static time_point now()
    { return estd::chrono::freertos_clock::now(); }

    // DEBT: This works well, but it would be nice for context to have clearer const
    // and non const separation
    struct context_type
    {
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

        static constexpr mutex::TimeoutModes mutex_timeout_mode()
        {
            return mutex::Abort;
        }
    };
};



// DEBT: Only bracketed out due to logging.  Consider using
// https://www.freertos.org/logging.html
#ifdef ESP_PLATFORM
// Specifically, FreeRTOS Task Notifier -
// not relevant for brute force polled approach
struct NotifierObserver
{
    static constexpr const char* TAG = "NotifyObserver";

    TaskHandle_t xSchedulerDaemon;
    // NOTE: Would consider time_point or similar here instead but a flag is equally efficient
    // and puts less demand on templating.  Also we expect schedule operations to be atomic, so
    // by convention one early_wakeup flag is sufficient.
    bool early_wakeup;

    NotifierObserver() :
        // DEBT: Optimization, would be nice to pass this in and also
        // have a 'strict' flag of sorts to optimize out boundary checks
        xSchedulerDaemon(NULLPTR),
        early_wakeup(false) {}

    //template <class TScheduler>
    //void on_notify(embr::internal::events::Scheduling<FreertosFunctorTraits> scheduling, TScheduler& scheduler)
    
    //template <class TContainer, class TImpl, class TSubject>
    //void on_notify(embr::internal::events::Scheduling<TImpl> scheduling,
    //    embr::internal::Scheduler<TContainer, TImpl, TSubject>& scheduler)

    template <class TScheduler>
    //void on_notify(embr::internal::events::Scheduling<typename TScheduler::impl_type> scheduling,
    void on_notify(typename TScheduler::event::scheduling scheduling,
        embr::internal::SchedulerContextBase<TScheduler>& context)
    {
        if((early_wakeup = scheduling.value.wake < context.scheduler().top_time()))
        {
            ESP_LOGV(TAG, "on_notify(scheduling) early wakeup tagged");
        }

        ESP_LOGD(TAG, "on_notify(scheduling)");
    }

    // 'bigger' one above consumes call, so this one doesn't get called.  Acceptable behavior
    template <class TImpl>
    void on_notify(embr::internal::events::Scheduling<TImpl> scheduling)
    {
        // Doing as a warning because we shouldn't see this ever, though
        // it may be benign if we do
        ESP_LOGW(TAG, "on_notify(scheduling) 2");
    }

    template <class TImpl>
    void on_notify(embr::internal::events::Scheduled<TImpl> scheduled)
    {
        if(early_wakeup)
        {
            // NOTE: Only doing warning temporarily as we build this out
            ESP_LOGD(TAG, "on_notify(scheduled) early wakeup");

            if(xSchedulerDaemon == NULLPTR)
            {
                ESP_LOGE(TAG, "on_notify(scheduled) failure - daemon not running, aborting wakeup");
                return;
            }
                
            // This will result in immediately waking up daemon, which we expect to turn right around
            // and sleep again - but for a shorter period of time.  Therefore, two wakes occur.
            xTaskNotifyGive(xSchedulerDaemon);
        }

        ESP_LOGV(TAG, "on_notify(scheduled)");
    }
};
#endif

struct FunctorImpl2 : FunctorImpl
{
    static constexpr const char* TAG = "FunctorImpl2";

    estd::freertos::wrapper::task daemon;
    bool early_wakeup;

    FunctorImpl2() :
        daemon(NULLPTR),
        early_wakeup(false) {}

    template <class TScheduler>
    void start(TScheduler* scheduler,
        configSTACK_DEPTH_TYPE usStackDepth = CONFIG_EMBR_FREERTOS_SCHEDULER_TASKSIZE,
        UBaseType_t uxPriority = CONFIG_EMBR_FREERTOS_SCHEDULER_PRIORITY)
    {
        TaskHandle_t t;
        BaseType_t result = xTaskCreate(embr::scheduler::freertos::notify_daemon_task<TScheduler>, "embr:scheduler2",
                                        usStackDepth, scheduler, uxPriority, &t);

        if(result != pdPASS)
        {
#ifdef ESP_PLATFORM
            ESP_LOGW(TAG, "Could not start scheduler daemon task");
#endif
        }

        daemon = t;
    }

    void stop()
    {
#ifdef INCLUDE_vTaskDelete
        vTaskDelete(daemon);
#endif
    }

    template <class T, class TScheduler>
    void on_scheduling(T& t, embr::internal::SchedulerContextBase<TScheduler>& context)
    {
        if((early_wakeup = get_time_point(t) < context.scheduler().top_time()))
        {
            ESP_LOGV(TAG, "on_notify(scheduling) early wakeup tagged");
        }

        ESP_LOGD(TAG, "on_notify(scheduling)");
    }

    template <class T, class TScheduler>
    void on_scheduled(T&, embr::internal::SchedulerContextBase<TScheduler>& context)
    {
        if(early_wakeup)
        {
            // NOTE: Only doing warning temporarily as we build this out
            ESP_LOGD(TAG, "on_notify(scheduled) early wakeup");

            if(daemon == NULLPTR)
            {
                ESP_LOGE(TAG, "on_notify(scheduled) failure - daemon not running, aborting wakeup");
                return;
            }
                
            // This will result in immediately waking up daemon, which we expect to turn right around
            // and sleep again - but for a shorter period of time.  Therefore, two wakes occur.
            daemon.notify_give();
        }

        ESP_LOGV(TAG, "on_notify(scheduled)");
    }
};

}}}
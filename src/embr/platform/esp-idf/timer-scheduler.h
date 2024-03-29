// All references pertain to README.md
// ISR-level Scheduler support for General Purpose timers [2]
#pragma once

#include <estd/port/freertos/event_groups.h>
#include <estd/mutex.h>

#include "rebase.h"
#include "timer.h"

#include "../../scheduler.h"
#include "../../internal/runtime-ratio.h"
#include "../../exp/platform/freertos/scheduler.h"

namespace embr { namespace esp_idf {

// DEBT: Wrap all this up in a templatized class
void timer_scheduler_init(Timer& timer, uint32_t divider, timer_isr_t, void*);
void timer_scheduler_tester();

}}

namespace embr { namespace scheduler { namespace esp_idf { namespace impl {

// Doesn't affect our mutex glitch at all, so keeping off
//#define EMBR_TIMER_TRACK_START 1

struct TimerBase : embr::internal::scheduler::impl::ReferenceBaseBase
{
    static constexpr const char* TAG = "impl::TimerBase";
    typedef embr::esp_idf::Timer timer_type;

    timer_type timer_;

    // This is a spinwait for the short duration that the isr is unable to acquire the mutex/binary semaphore
    // doing this effectively grants ISR higher priority to get the mutex because everyone else has to wait
    // for this variable to be false
    volatile bool isr_acquiring_mutex_ = false;

    // Represents a kind of "clear to send" - in other words, if bit 1 is set,
    // normal scheduling may proceed.  if bit 1 is clear, that means ISR is busy
    estd::freertos::event_group<true> event_;

    static constexpr EventBits_t event_clear_to_schedule = 1;

    // this and event_ are alternatives to above spinlock.  However, mechanism is not fully used yet
    inline bool wait_for_isr(estd::chrono::freertos_clock::duration wait)
    {
        return event_.wait_bits(event_clear_to_schedule, false, true, wait);
    }

#if EMBR_TIMER_TRACK_START
    // EXPERIMENTAL -
    // 1) I would think we could interrogate esp-idf API for this
    // 2) I am not sure we need to track this but maybe, getting an odd crash when starting an already-started timer
    bool is_timer_started_ = false;
#endif

    inline timer_type& timer() { return timer_; }
    constexpr const timer_type& timer() const { return timer_; }

    TimerBase(const timer_type& timer) : timer_{timer} {}
    TimerBase(timer_group_t group, timer_idx_t idx) : timer_(group, idx) {}

    struct mutex : embr::scheduler::freertos::timed_mutex<true, true>
    {
        typedef embr::scheduler::freertos::timed_mutex<true, true> base_type;

        template <class TScheduler>
        inline void lock(embr::internal::SchedulerContextBase<TScheduler>& context)
        {
            if(context.in_isr())
            {
                while(context.scheduler().isr_acquiring_mutex_)
                    estd::this_thread::yield();
            }

            base_type::lock(context);
        }
    };

private:
    template <class TScheduler>
    static bool timer_callback(void* arg);

    // Like the old helper, but semi-cheats so that we can get to protected members
    // Not 100% convinced this is a good idea - 'Buddy' class is way better
    // NOTE: DO NOT add any member variables or virtual methods to this class!
    // Not performing any function other than proving a concept
    template <class TScheduler>
    struct Wrapper : TScheduler
    {
        typedef TScheduler this_type;
        typedef typename this_type::container_type container_type;
        typedef typename embr::internal::Rebaser<container_type> rebaser_type;
        typedef typename rebaser_type::duration duration;

        bool timer_callback();
    };

    template <class TScheduler>
    bool timer_callback(TScheduler& scheduler);

protected:
    template <class TScheduler, class TDuration>
    void rebase(TScheduler& scheduler, TDuration);

    // NOTE: Beware, do not name 'context_type' because impl already has that reserved for
    // impl-specialized context
    template <class TScheduler>
    using scheduler_context_type = embr::internal::SchedulerContextBase<TScheduler>;

    /// Do a rebase if we've met near-overflow criteria
    /// @return true native timebase of next event, with no offset on it
    /// @remarks undefined if there are no scheduled events
    template <class TScheduler>
    uint64_t rebase_eval(TScheduler& scheduler, scheduler_context_type<TScheduler>& context, uint64_t native_now);

    // We pass this in to avoid downcasting
    template <class TScheduler>
    // for example, 80 = prescaler for 1 MHz clock - remember, we're dividing
    // "default is APB_CLK running at 80 MHz"
    void start(TScheduler* scheduler, uint32_t divider);

    inline uint64_t get_counter(bool in_isr) const
    {
        if(in_isr)
            return timer().get_counter_value_in_isr();
        else
        {
            uint64_t v;
            ESP_ERROR_CHECK(timer().get_counter_value(&v));
            return v;
        }
    }

public:
    // DEBT: Should be protected
    // DEBT: Since I can't easily reset timer to 0, we're tracking an offset to subtract down
    // to a relative zero
    uint64_t native_offset = 0;

    // DEBT: should be protected - impl() trick interrupts that
    // DEBT: This in particular is a better candidate for subject/observer
    template <class T, class Rep, class Period, class TContext>
    inline void on_processing(T&, estd::chrono::duration<Rep, Period> current, TContext)
    {
        ESP_DRAM_LOGV(TAG, "on_processing: current=%llu", current.count());
    }
};




template <class T, int divider_ = -1,
    typename TTimePoint = typename estd::remove_pointer<T>::type::time_point,
    class TReference = embr::internal::scheduler::impl::Reference<T, TTimePoint> >
struct Timer;

template <class T, int divider_, typename TTimePoint, class TReference>
struct Timer : 
    TimerBase,
    embr::internal::DurationConverter<uint64_t, divider_, embr::esp_idf::Timer::base_clock_hz()>
{
    static constexpr const char* TAG = "impl::Timer";

    typedef TimerBase base_type;
    using typename base_type::timer_type;
    typedef embr::internal::scheduler::impl::TimePointTraits<TTimePoint> helper_type;
    typedef typename helper_type::time_point time_point;
    typedef typename helper_type::duration duration;
    typedef embr::internal::DurationConverter<uint64_t, divider_, timer_type::base_clock_hz()> converter_type;

    struct context_type : embr::scheduler::freertos::context_type
    {
        // When in an ISR...

        // ... this value represents the entry time of the ISR in native ticks
        uint64_t initial_counter;

        // ... this value represents the current time in offset timebase
        time_point current_time;

        // DEBT: Do specialized ISR context since these only populate in ISR scenarios
    };

    typedef embr::scheduler::freertos::context_factory context_factory;


    // reference_impl comes in handy for supporting both value and pointer of T.  Also
    // if one *really* wants to deviate from 'event_due' and 'process' paradigm, it's done
    // with a custom reference_impl.
    // DEBT: Break out those static/support portions since Reference is designed to be a complete
    // base class
    typedef TReference reference_impl;
    typedef typename reference_impl::value_type value_type;

    static constexpr bool is_runtime_divider() { return divider_ == -1; }
    const converter_type& duration_converter() const { return *this; }

    // Get maximum number of timer ticks we can accumulate before rolling over scheduler's timebase
    uint64_t overflow_max() const
    {
        return duration_converter().convert(helper_type::time_point_max());
    }

    inline time_point now(bool in_isr = false)
    {
        time_point v;
        uint64_t counter = base_type::get_counter(in_isr);
        duration_converter().convert(counter - base_type::native_offset, &v);
        return v;
    }

    timer_type& timer() { return base_type::timer(); }

    // Indicates how much further in the future we should wake up and try to acquire binary semaphore
    // again
    // 1.  1000 ticks is too arbitrary and should be more configurable or at least deduced better
    // 2.  in non-diagnostic-logging scenarios the simpler + to initial_counter works well and is better
    static constexpr uint64_t get_spinwait_wakeup(uint64_t now)
    {
        return now + 1000;
    }

    // EXPERIMENTAL - different way of overcoming overflow/rollover
    // let it happen on time point, since the main importance is that scheduling sorts things right
    // time_point_offset can help that by subtracting back down when grabbing time point
    // NOTE: Can't use this yet because Compare class in Scheduler needs get_time_point to be static
    // NOTE: This technique isn't 100% better than a rebase, because the rebase only does the extra subtractions
    // once in a while - while this does it all the time
    duration time_point_offset = helper_type::duration_zero();

    static inline time_point get_time_point(const value_type& v)
    {
        return reference_impl::get_time_point(v);
    }

    static inline bool process(value_type& v, time_point current_time)
    {
        return reference_impl::process(v, current_time);
    }

    template <class TScheduler, int shadowed = divider_,
        typename = typename estd::enable_if<shadowed == -1>::type >
    void start(TScheduler* scheduler, uint32_t divider)
    {
        // DEBT: Sloppy way of initializing numerator
        this->numerator(divider);
        base_type::start(scheduler, divider);
    }

    template <class TScheduler, int shadowed = divider_,
        typename = typename estd::enable_if<shadowed != -1>::type >
    void start(TScheduler* scheduler)
    {
        base_type::start(scheduler, divider_);
    }

    template <class TScheduler>
    void on_scheduled(const value_type& value, const scheduler_context_type<TScheduler>&);

    template <class TScheduler>
    void on_processed(const value_type* value, time_point, const scheduler_context_type<TScheduler>&);

    constexpr Timer(const timer_type& timer) : base_type{timer} {}
    constexpr Timer(timer_group_t group, timer_idx_t idx) : base_type(group, idx) {}
};


struct TimerSchedulerObserver
{
    static constexpr const char* TAG = "TimerSchedulerObserver";

    bool early_wakeup;

    template <class TContainer, class TImpl, class TSubject>
    void on_notify(embr::internal::events::Scheduling<TImpl> scheduling,
        embr::internal::Scheduler<TContainer, TImpl, TSubject>& scheduler)
    {

    }
};

/*
template <class TScheduler>
class TimerScheduler
{
    typedef TimerScheduler this_type;
    typedef TScheduler scheduler_type;
    typedef typename scheduler_type::impl_type impl_type;
    typedef typename impl_type::value_type value_type;
    typedef typename scheduler_type::time_point time_point;

private:
    scheduler_type scheduler;

    Timer& timer() { return scheduler.timer(); }

public:
    TimerScheduler(const Timer& timer) :
        scheduler{embr::internal::scheduler::impl_params_tag{}, timer} {}

    void init() { scheduler.init(&scheduler); }

    // DEBT: do_notify_scheduling doesn't take a const, so this can't either
    void schedule(value_type& v);
}; */


}}}}

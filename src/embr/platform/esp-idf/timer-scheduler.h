#pragma once

#include <estd/mutex.h>

#include "timer.h"

#include "../../scheduler.h"
#include "../../exp/runtime-chrono.h"
#include "../../exp/platform/freertos/scheduler.h"

namespace embr { namespace esp_idf {

template <typename TInt>
struct DurationImplBase
{
    typedef TInt time_point;

    // DEBT: Make this the source timer
    Timer* timer_;
    Timer& timer() { return *timer_; }

    inline TInt now(bool in_isr = false)
    {
        if(in_isr)
            return (TInt)timer().get_counter_value_in_isr();
        else
        {
            uint64_t v;
            ESP_ERR_CHECK(timer().get_counter_value(&v));
            return (TInt) v;
        }
    }
};

template <typename TInt, int divisor_ = -1>
struct DurationImpl2 : DurationImplBase<TInt>
{
    typedef DurationImplBase<TInt> base_type;

    static constexpr unsigned apb_clock() { return 80000000; }
    static constexpr unsigned divisor() { return divisor_; }

    typedef estd::chrono::duration<TInt, estd::ratio<divisor(), apb_clock()> > duration;

    template <typename Rep, typename Period>
    static constexpr duration convert(const estd::chrono::duration<Rep, Period>& convert_from)
    {
        return duration(convert_from);
    }
};


template <typename TInt>
struct DurationImpl2<TInt, -1> : 
    DurationImplBase<TInt>,
    embr::experimental::TimerSchedulerConverter
{
    typedef DurationImplBase<TInt> base_type;
};

/*
NOTE: Don't think we'll do offset in any case
// 64-bit native versions don't do offset
template <int divisor>
struct DurationImpl2<uint64_t, divisor>
{

};


// 64-bit native versions don't do offset
template <>
struct DurationImpl2<uint64_t, -1> : embr::experimental::TimerSchedulerConverter
{

};
*/


struct DurationImpl : embr::internal::scheduler::impl::ReferenceBaseBase
{
    Timer timer_;

    inline Timer& timer() { return timer_; }
    constexpr const Timer& timer() const { return timer_; }

    typedef estd::chrono::duration<uint32_t, estd::micro> time_point;

    struct value_type
    {
        //embr::detail::Debouncer* debouncer;
        time_point wakeup;

        value_type(time_point wakeup) : wakeup{wakeup} {}
        value_type() = default;
    };

    static inline const time_point& get_time_point(const value_type& v)
    {
        return v.wakeup;
    }

    static bool process(value_type& v, time_point now)
    {
        return false;
    }

    void init();

    typedef embr::freertos::experimental::FunctorImpl::mutex mutex;
    typedef embr::freertos::experimental::FunctorImpl::context_type context_type;
    typedef embr::freertos::experimental::FunctorImpl::context_factory context_factory;

    DurationImpl(const Timer& timer) : timer_{timer} {}
};

// DEBT: Wrap all this up in a templatized class
void timer_scheduler_init(Timer& timer, uint32_t divider, timer_isr_t, void*);
void timer_scheduler_tester();

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

    void timer_callback();
    static bool timer_callback(void* arg);

    Timer& timer() { return scheduler.timer(); }

public:
    TimerScheduler(const Timer& timer) :
        scheduler{embr::internal::scheduler::impl_params_tag{}, timer} {}

    void init();

    // DEBT: do_notify_scheduling doesn't take a const, so this can't either
    void schedule(value_type& v);
};


}}
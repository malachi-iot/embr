#pragma once

#include <estd/mutex.h>

#include "timer.h"

#include "../../scheduler.h"
#include "../../exp/runtime-chrono.h"
#include "../../exp/platform/freertos/scheduler.h"

namespace embr { namespace esp_idf {

struct DurationImplBaseBase : embr::internal::scheduler::impl::ReferenceBaseBase
{
    static constexpr const char* TAG = "DurationImplBaseBase";

    Timer timer_;

    inline Timer& timer() { return timer_; }
    constexpr const Timer& timer() const { return timer_; }

    constexpr DurationImplBaseBase(const Timer& timer) : timer_{timer} {}
    constexpr DurationImplBaseBase(timer_group_t group, timer_idx_t idx) : timer_(group, idx) {}

    typedef embr::freertos::experimental::FunctorImpl::mutex mutex;
    typedef embr::freertos::experimental::FunctorImpl::context_type context_type;
    typedef embr::freertos::experimental::FunctorImpl::context_factory context_factory;

private:
    template <class TScheduler>
    static bool timer_callback(void* arg);

    // Like the old helper, but semi-cheats so that we can get to protected members
    // Not 100% convinced this is a good idea
    // NOTE: DO NOT add any member variables or virtual methods to this class!
    template <class TScheduler>
    struct Wrapper : TScheduler
    {
        typedef TScheduler this_type;

        bool timer_callback();
    };

    template <class TScheduler>
    bool timer_callback(TScheduler& scheduler);

protected:
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
    uint64_t offset = 0;

    // DEBT: should be protected - impl() trick interrupts that
    // DEBT: This in particular is a better candidate for subject/observer
    template <class T, class Rep, class Period, class TContext>
    inline void on_processing(T&, estd::chrono::duration<Rep, Period> current, TContext)
    {
        ESP_DRAM_LOGV(TAG, "on_processing: current=%llu", current.count());
    }
};


template <typename TTimePoint>
struct DurationImplBase;

template <typename TInt>
struct DurationImplBase : DurationImplBaseBase
{
    typedef TInt time_point;
    typedef TInt int_type;

    static constexpr bool is_chrono() { return false; }

    constexpr DurationImplBase(const Timer& timer) : DurationImplBaseBase{timer} {}
    constexpr DurationImplBase(timer_group_t group, timer_idx_t idx) : DurationImplBaseBase(group, idx) {}
};

// DEBT: Move some of this specialization magic out to Reference
template <typename Rep, typename Period>
struct DurationImplBase<estd::chrono::duration<Rep, Period> > : DurationImplBaseBase
{
    typedef estd::chrono::duration<Rep, Period> duration;
    typedef duration time_point;
    typedef Rep int_type;

    static constexpr bool is_chrono() { return true; }

    constexpr DurationImplBase(const Timer& timer) : DurationImplBaseBase{timer} {}
    constexpr DurationImplBase(timer_group_t group, timer_idx_t idx) : DurationImplBaseBase(group, idx) {}
};

template <class T, int divider_ = -1,
    typename TTimePoint = typename std::remove_pointer<T>::type::time_point,
    class TReference = embr::internal::scheduler::impl::Reference<T, TTimePoint> >
struct DurationImpl2;

template <class T, int divider_, typename TTimePoint, class TReference>
struct DurationImpl2 : DurationImplBase<TTimePoint>,
    embr::experimental::DurationConverter<
        typename DurationImplBase<TTimePoint>::int_type, divider_, Timer::base_clock_hz()>
{
    static constexpr const char* TAG = "DurationImpl2";

    typedef DurationImplBase<TTimePoint> base_type;
    typedef typename base_type::time_point time_point;
    typedef embr::experimental::DurationConverter<typename base_type::int_type, divider_, Timer::base_clock_hz()> converter_type;

    // reference_impl comes in handy for supporting both value and pointer of T.  Also
    // if one *really* wants to deviate from 'event_due' and 'process' paradigm, it's done
    // with a custom reference_impl.
    // DEBT: Break out those static/support portions since Reference is designed to be a complete
    // base class
    typedef TReference reference_impl;
    typedef typename reference_impl::value_type value_type;

    static constexpr bool is_runtime_divider() { return divider_ == -1; }
    const converter_type& duration_converter() const { return *this; }

    inline time_point now(bool in_isr = false)
    {
        time_point v;
        uint64_t counter = base_type::get_counter(in_isr);
        duration_converter().convert(counter - base_type::offset, &v);
        return v;
    }

    Timer& timer() { return base_type::timer(); }

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
        this->numerator_ = divider;
        base_type::start(scheduler, divider);
    }

    template <class TScheduler, int shadowed = divider_,
        typename = typename estd::enable_if<shadowed != -1>::type >
    void start(TScheduler* scheduler)
    {
        base_type::start(scheduler, divider_);
    }

    // NOTE: Beware, do not name 'context_type' because impl already has that reserved for
    // impl-specialized context
    template <class TScheduler>
    using scheduler_context_type = embr::internal::SchedulerContextBase<TScheduler>;

    template <class TScheduler>
    void on_scheduled(const value_type& value, const scheduler_context_type<TScheduler>&);

    template <class TScheduler>
    void on_processed(const value_type* value, time_point, const scheduler_context_type<TScheduler>&);

    constexpr DurationImpl2(const Timer& timer) : base_type{timer} {}
    constexpr DurationImpl2(timer_group_t group, timer_idx_t idx) : base_type(group, idx) {}
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


}}
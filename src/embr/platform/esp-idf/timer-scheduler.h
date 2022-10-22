#pragma once

#include "timer.h"

#include "../../scheduler.h"

namespace embr { namespace esp_idf {

struct DurationImpl
{
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

    typedef estd::monostate mutex;
};

// DEBT: Wrap all this up in a templatized class
void timer_scheduler_init(Timer& timer, uint32_t divider, timer_isr_t, void*);
void timer_scheduler_tester();

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

public:
    Timer timer;

    TimerScheduler(const Timer& timer) : timer{timer} {}

    void init();

    // DEBT: do_notify_scheduling doesn't take a const, so this can't either
    void schedule(value_type& v);
};


}}
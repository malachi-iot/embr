#pragma once

#include "timer-scheduler.h"

namespace embr { namespace esp_idf {


static auto last_now = estd::chrono::esp_clock::now();

template <class TScheduler>
inline IRAM_ATTR void TimerScheduler<TScheduler>::timer_callback ()
{
    uint64_t counter;

    //ets_printf("0 TimerScheduler Intr: group=%d, idx=%d\n", timer.group, timer.idx);

    counter = timer.get_counter_value_in_isr();

    auto now = estd::chrono::esp_clock::now();
    auto duration = now - last_now;

    ets_printf("1 TimerScheduler Intr: duration=%lldus, timer_counter=%lld, this=%p\n",
        duration.count(), counter, this);

    scheduler.process(time_point(counter));

    // DEBT: Consider making a scheduler.empty()
    bool more = scheduler.size() > 0;
    if(more)
    {
        time_point next = scheduler.top_time();
        //next += duration;
        // FIX: "works" but causes a mega loop
        // I believe API itself is functioning I am just not using it right
        //timer.set_alarm_value_in_isr(next.count());
        ets_printf("1.1 TimerScheduler Intr: size=%d, next=%lluus\n",
            scheduler.size(),
            next.count());
    }
    else
    {
        ets_printf("1.2 TimerScheduler Intr: no further events\n");
    }
}

template <class TScheduler>
bool IRAM_ATTR TimerScheduler<TScheduler>::timer_callback (void *param)
{
    //timer.pause();

    static_cast<this_type*>(param)->timer_callback();

    // DEBT: Pretty sure we need an ISR-friendly version of this.  However, I can't find one
    //timer_set_alarm(timer_group, timer_idx, TIMER_ALARM_DIS);
    // Above not needed, becase
    // "Once triggered, the alarm is disabled automatically and needs to be re-enabled to trigger again."

    return false;
}


template <class TScheduler>
void TimerScheduler<TScheduler>::init()
{
    // Set prescaler for 1 MHz clock - remember, we're dividing
    // "default is APB_CLK running at 80 MHz"
    uint32_t divider = 80;

    timer_scheduler_init(timer, divider, &timer_callback, this);
}

template <class TScheduler>
inline void TimerScheduler<TScheduler>::schedule(value_type& v)
{
    //timer.enable_alarm_in_isr();
    time_point t = impl_type::get_time_point(v);
    estd::chrono::duration<uint64_t, estd::micro> native;

    native = t;

    timer.set_alarm(TIMER_ALARM_EN);
    timer.set_alarm_value(1000000);    // FIX: Dummy value - set to 1s
    timer.start();

    scheduler.schedule(v);
}


}} // namespace embr { namespace esp_idf 
#pragma once

#include "timer-scheduler.h"

namespace embr { namespace esp_idf {


static auto last_now = estd::chrono::esp_clock::now();

template <class TScheduler>
inline void TimerScheduler<TScheduler>::timer_callback ()
{
    // FIX: Mysteriously, this call crashes our timer
    //uint64_t counter = timer.get_counter_value_in_isr();
    uint64_t counter = 0;
    auto now = estd::chrono::esp_clock::now();
    auto duration = now - last_now;

    ets_printf("1 TimerScheduler Intr, duration=%lldus, timer_counter=%lld\n",
        duration.count(), counter);

    scheduler.process(time_point(counter));
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
    timer_scheduler_init(timer, &timer_callback, this);
}

template <class TScheduler>
inline void TimerScheduler<TScheduler>::schedule(value_type& v)
{
    //timer.enable_alarm_in_isr();
    timer.set_alarm(TIMER_ALARM_EN);
    timer.set_alarm_value(1000000);    // FIX: Dummy value - set to 1s
    timer.start();

    scheduler.schedule(v);
}


}} // namespace embr { namespace esp_idf 
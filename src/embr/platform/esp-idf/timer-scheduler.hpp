#pragma once

#include "timer-scheduler.h"
#include "../../exp/runtime-chrono.h"

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

    assert(mutex.native_handle().take_from_isr(nullptr));
    scheduler.process(time_point(counter));
    assert(mutex.native_handle().give_from_isr(nullptr));

    // DEBT: Consider making a scheduler.empty()
    // DEBT: We need mutex protection down here too
    bool more = scheduler.size() > 0;
    if(more)
    {
        time_point next = scheduler.top_time();

        embr::experimental::TimerSchedulerConverter converter;

        uint64_t native = converter.convert(next);

        ets_printf("1.1 TimerScheduler Intr: size=%d, next=%lluus\n",
            scheduler.size(),
            next.count());

        timer.set_alarm_value_in_isr(native);
    }
    else
    {
        ets_printf("1.2 TimerScheduler Intr: no further events\n");
        //timer.pause();
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

    embr::experimental::TimerSchedulerConverter converter;

    uint64_t native = converter.convert(t);

    assert(mutex.try_lock_for(estd::chrono::milliseconds(50)));
    scheduler.schedule(v);
    assert(mutex.unlock());

    last_now = estd::chrono::esp_clock::now();

    if(scheduler.size() == 1)
    {
        timer.set_alarm(TIMER_ALARM_EN);
        timer.set_alarm_value(native);
        timer.start();
    }
}


}} // namespace embr { namespace esp_idf 
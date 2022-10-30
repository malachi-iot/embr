#pragma once

#include <driver/timer.h>

#include "../../detail/debounce.h"
#include "gpio.h"
#include "queue.h"

#include "timer-scheduler.h"

namespace embr { namespace esp_idf {

class Debouncer;


namespace internal {

struct Item
{
    typedef estd::chrono::milliseconds duration;
    typedef duration time_point;

    detail::Debouncer debouncer_;
    Debouncer* parent_;
    gpio pin_;
    bool low_means_pressed = true;
    duration last_wakeup_;
    duration wakeup_;

    detail::Debouncer& debouncer() { return debouncer_; }
    const detail::Debouncer& debouncer() const { return debouncer_; }
    const gpio& pin() const { return pin_; }
    time_point event_due() const { return wakeup_; }

    //Item() = default;
    Item(const Item& copy_from) = default;
    Item(Debouncer* parent, gpio pin) : parent_{parent}, pin_{pin} {}
};




template <int divider_ = 80>
struct ThresholdImpl : DurationImpl2<Item*, divider_>
{
    static constexpr const char* TAG = "ThreadholdImpl";

    typedef DurationImpl2<Item*, divider_> base_type;
    using typename base_type::value_type;
    using typename base_type::time_point;

    // we're scheduled to reach here optimisitcally thinking up or down energy is high
    // enough to yield a state change
    bool process(value_type v, time_point now);

    constexpr ThresholdImpl(const Timer& timer) : base_type{timer} {}
    constexpr ThresholdImpl(timer_group_t group, timer_idx_t idx) : base_type(group, idx) {}
};



}


class Debouncer
{
    typedef internal::Item item_type;

public:
    enum States
    {
        Uninitialized = -1,
        Up = 0,
        Down = 1,
        Held
    };

    struct Notification
    {
        const int pin;
        const States state;

        Notification(int pin = -1, States state = Uninitialized) : 
            pin(pin),
            state{state}
        {}
    };
    
private:
    gpio_isr_handle_t gpio_isr_handle;

    void gpio_isr();
    void timer_group0_isr();

    static void gpio_isr(void*);
#if UNUSED
    void timer_init(bool callback_mode);
    static void timer_group0_isr(void*);
    static bool timer_group0_callback(void *param);
#endif

    embr::internal::layer1::Scheduler<5, internal::ThresholdImpl<> > scheduler;

public:
    // DEBT: Don't expose as public - however, most of these will be in the impl eventually
    // anyhow
    void emit_state(const item_type& item);
    // DEBT: Not sure I want to expose the whole queue here, but seems OK
    embr::freertos::layer1::queue<Notification, 10> queue;

public:
    Debouncer(timer_group_t, timer_idx_t);
    ~Debouncer();

    void track(int pin);
};

const char* to_string(Debouncer::States state);

}}
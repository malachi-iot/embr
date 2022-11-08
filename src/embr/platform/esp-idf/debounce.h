#pragma once

#include <driver/timer.h>

#include <estd/port/freertos/queue.h>

#include "../../detail/debounce.h"
#include "gpio.h"


#include "timer-scheduler.h"

namespace embr { 
    
namespace esp_idf {

class Debouncer;

}

namespace scheduler { namespace esp_idf { namespace impl {

// FIX: Naming - should be DebounceControlStructure or similar
struct Item
{
    typedef estd::chrono::milliseconds duration;
    typedef duration time_point;

    detail::Debouncer debouncer_;
    embr::esp_idf::Debouncer* parent_;
    embr::esp_idf::gpio pin_;
    bool low_means_pressed = true;
    duration last_wakeup_;
    duration wakeup_;

    detail::Debouncer& debouncer() { return debouncer_; }
    const detail::Debouncer& debouncer() const { return debouncer_; }
    const embr::esp_idf::gpio& pin() const { return pin_; }
    time_point event_due() const { return wakeup_; }

    //Item() = default;
    Item(const Item& copy_from) = default;
    Item(embr::esp_idf::Debouncer* parent, embr::esp_idf::gpio pin) : parent_{parent}, pin_{pin} {}

    inline void rebase(duration v)
    {
        last_wakeup_ -= v;       // DEBT: Could get an underflow here, though probably thats OK
        wakeup_ -= v;
    }
};




template <int divider_ = 80>
struct Threshold : embr::scheduler::esp_idf::impl::Timer<Item*, divider_>
{
    static constexpr const char* TAG = "impl::Threshhold";

    typedef embr::scheduler::esp_idf::impl::Timer<Item*, divider_> base_type;
    using typename base_type::value_type;
    using typename base_type::time_point;

    // we're scheduled to reach here optimisitcally thinking up or down energy is high
    // enough to yield a state change
    bool process(value_type v, time_point now);

    constexpr Threshold(const embr::esp_idf::Timer& timer) : base_type{timer} {}
    constexpr Threshold(timer_group_t group, timer_idx_t idx) : base_type(group, idx) {}
};



}}}

namespace esp_idf {

class Debouncer
{
    typedef scheduler::esp_idf::impl::Item item_type;

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

    embr::internal::layer1::Scheduler<5,
        scheduler::esp_idf::impl::Threshold<>
        > scheduler;

public:
    // DEBT: Don't expose as public - however, most of these will be in the impl eventually
    // anyhow
    void emit_state(const item_type& item);
    // DEBT: Not sure I want to expose the whole queue here, but seems OK
    estd::freertos::layer1::queue<Notification, 10> queue;

public:
    Debouncer(timer_group_t, timer_idx_t);
    ~Debouncer();

    void track(int pin);
};

const char* to_string(Debouncer::States state);

}}
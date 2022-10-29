#pragma once

#include <driver/timer.h>

#include "../../detail/debounce.h"
#include "gpio.h"
#include "queue.h"

namespace embr { namespace esp_idf {


class Debouncer;

struct Item
{
    typedef estd::chrono::milliseconds duration;
    typedef duration time_point;

    detail::Debouncer debouncer_;
    gpio pin_;
    bool low_means_pressed = true;
    duration wakeup_;
    Debouncer* parent_;

    detail::Debouncer& debouncer() { return debouncer_; }
    const detail::Debouncer& debouncer() const { return debouncer_; }
    gpio pin() const { return pin_; }
    time_point event_due() const { return wakeup_; }

    bool on() const
    {
        return pin_.level();
    }

    //Item() = default;
    Item(const Item& copy_from) = default;
    Item(Debouncer* parent, gpio pin) : pin_{pin} {}
};



class Debouncer
{
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

    void timer_init(bool callback_mode);
    void gpio_isr();
    void timer_group0_isr();
    void emit_state(const Item& item);

    static void gpio_isr(void*);
#if UNUSED
    static void timer_group0_isr(void*);
    static bool timer_group0_callback(void *param);
#endif

public:
    // DEBT: Not sure I want to expose the whole queue here, but seems OK
    embr::freertos::layer1::queue<Notification, 10> queue;

public:
    Debouncer(bool callback_mode);
    ~Debouncer();

    void track(int pin);
};

const char* to_string(Debouncer::States state);

}}
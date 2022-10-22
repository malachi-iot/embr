#pragma once

#include <driver/gpio.h>
#include <driver/timer.h>

#include "../../detail/debounce.h"
#include "queue.h"

namespace embr { inline namespace esp_idf {


struct Item
{
    detail::Debouncer debouncer_;
    int pin_;
    bool low_means_pressed = true;

    detail::Debouncer& debouncer() { return debouncer_; }
    const detail::Debouncer& debouncer() const { return debouncer_; }
    int pin() const { return pin_; }

    bool on() const
    {
        return false;
    }

    Item() = default;
    Item(const Item& copy_from) = default;
    Item(int pin) : pin_{pin} {}
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

    void timer_init();
    void gpio_isr();
    void timer_group0_isr();
    void emit_state(const Item& item);

    static void gpio_isr(void*);
    static void timer_group0_isr(void*);
    static bool timer_group0_callback(void *param);

public:
    // DEBT: Not sure I want to expose the whole queue here, but seems OK
    embr::freertos::layer1::queue<Notification, 10> queue;

public:
    Debouncer();
    ~Debouncer();

    void track(int pin);
};

const char* to_string(Debouncer::States state);

}}
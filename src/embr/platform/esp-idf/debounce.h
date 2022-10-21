#pragma once

#include "../../detail/debounce.h"
#include "queue.h"

namespace embr { inline namespace esp_idf {

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
        const States state;

        Notification(States state = Uninitialized) : 
            state{state}
        {}
    };
    
private:
    void timer_init();
    void gpio_isr();
    void timer_group0_isr();
    void emit_state(const embr::detail::Debouncer& d);

    static void gpio_isr(void*);
    static void timer_group0_isr(void*);
    static bool timer_group0_callback(void *param);

public:
    // DEBT: Not sure I want to expose the whole queue here, but seems OK
    embr::freertos::queue<Notification> queue;

public:
    Debouncer();
    ~Debouncer();

    void track(int pin);
};

const char* to_string(Debouncer::States state);

}}
#pragma once

#include "../../detail/debounce.h"
#include "queue.h"

namespace embr { inline namespace esp_idf {

class Debouncer
{
public:
    struct Notification
    {
        int val;
    };
    
private:
    void timer_init();
    void gpio_isr();
    void timer_group0_isr();

    static void gpio_isr(void*);
    static void timer_group0_isr(void*);

public:
    // DEBT: Not sure I want to expose the whole queue here, but seems OK
    embr::freertos::queue<Notification> queue;

public:
    Debouncer();
    ~Debouncer();

    void track(int pin);
};

}}
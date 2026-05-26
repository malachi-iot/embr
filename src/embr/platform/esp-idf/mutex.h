#pragma once

#include <freertos/task.h>

namespace embr { namespace esp_idf {

struct hw_mutex
{
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

    bool lock()
    {
        taskENTER_CRITICAL(&mux);
        return true;
    }

    void unlock()
    {
        taskEXIT_CRITICAL(&mux);
    }
};


struct hw_mutex_isr
{
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

    bool lock()
    {
        taskENTER_CRITICAL_ISR(&mux);
        return true;
    }

    void unlock()
    {
        taskEXIT_CRITICAL_ISR(&mux);
    }
};

}}

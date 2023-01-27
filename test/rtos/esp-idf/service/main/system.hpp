#pragma once

#include "system.h"

template <class TSubject, class TImpl>
void SystemService::runtime<TSubject, TImpl>::sleep()
{
    base_type::state(Sleeping);
    esp_deep_sleep_start();
    // Never reaches here, so no point in a 'Slept' state
}


template <class TSubject, class TImpl>
bool SystemService::runtime<TSubject, TImpl>::wake()
{
    event::wake e{esp_sleep_get_wakeup_cause()};

    switch (e.cause)
    {
        case ESP_SLEEP_WAKEUP_UNDEFINED:
            return false;

        // All others are some kind of light or deep sleep wakeup
        default: break;
    }

    base_type::notify(e);
    return true;
}

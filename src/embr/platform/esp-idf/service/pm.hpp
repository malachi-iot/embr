#pragma once

#include "pm.h"

namespace embr::esp_idf {

namespace service { inline namespace v1 {

template <class TSubject, class TImpl>
void PowerManager::runtime<TSubject, TImpl>::sleep()
{
    base_type::state(Sleeping);
    esp_deep_sleep_start();
    // Never reaches here, so no point in a 'Slept' state
}


template <class TSubject, class TImpl>
void PowerManager::runtime<TSubject, TImpl>::sleep_for(estd::chrono::microseconds duration)
{
    base_type::notify(event::sleep_for{duration});
    base_type::state(Sleeping);
    esp_deep_sleep(duration.count());
    // Never reaches here, so no point in a 'Slept' state
}


template <class TSubject, class TImpl>
bool PowerManager::runtime<TSubject, TImpl>::wake()
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

}}

}
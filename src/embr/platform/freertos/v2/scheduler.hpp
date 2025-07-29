#pragma once

#include "scheduler.h"

namespace embr { namespace scheduler { namespace freertos { inline namespace v1 {

namespace detail {

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
BaseType_t scheduler_with_notify<Traits, Container>::wait(TickType_t ticks_to_wait)
{
    uint32_t v;

    return xTaskNotifyWaitIndexed(0, 0, 0, &v, ticks_to_wait);
}


}

}}}}

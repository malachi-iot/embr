#pragma once

#include "../../../internal/v2/scheduler.h"
#include "../gptimer.h"

namespace embr { namespace scheduler { namespace esp_idf { inline namespace v1 {

// TODO: This is where gptimer-isr-timed scheduler will live

namespace detail {

}

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
class gptimer_scheduler : public scheduler::v1::detail::scheduler<Traits, Container>
{
    using timer_type = embr::esp_idf::gptimer;

public:
};

}}}}

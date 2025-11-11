#pragma once

#include <estd/port/freertos/event_group.h>
#include <estd/port/freertos/mutex.h>
#include <estd/port/freertos/wrapper/task.h>
#include <estd/port/freertos/chrono.h>

#include "../../../internal/v2/scheduler.h"

namespace embr { namespace scheduler { namespace freertos { inline namespace v1 {

namespace detail {

class scheduler_base2
{
protected:  
#if configSUPPORT_STATIC_ALLOCATION
    static constexpr bool static_alloc = true;
#else
    static constexpr bool static_alloc = false;
#endif

public:
    using clock_type = estd::chrono::freertos_clock;
    using time_point = typename clock_type::time_point;
    using duration = typename clock_type::duration;
};

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
class scheduler_base : public scheduler::v1::detail::scheduler<Traits, Container>
{
    using base_type = scheduler::v1::detail::scheduler<Traits, Container>;

public:

    // DEBT: This overrides time_point from base class
    // DEBT: For now, time_point from Traits, base_type and here must all belong
    // to freertos_clock
    using clock_type = estd::chrono::freertos_clock;
    using time_point = typename clock_type::time_point;
    using duration = typename clock_type::duration;
};


template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
class scheduler_with_notify : public scheduler_base<Traits, Container>,
    scheduler_base2
{
    using base_type = scheduler_base<Traits, Container>;
    using task_type = estd::freertos::wrapper::task;
    using mutex_type = estd::freertos::mutex<static_alloc>;

    using typename base_type::clock_type;
    using typename base_type::duration;
    using typename base_type::time_point;
    using typename base_type::const_reference;
    using typename base_type::process_result;

    mutex_type mutex_;
    task_type task_;

public:
    scheduler_with_notify() : task_(task_type::current())   {}

    using typename base_type::pointer;

    void reschedule(pointer item)
    {
        if(base_type::reschedule(item, mutex_))
        {
            task_.notify(1, eSetBits);
        }
    }

    void reschedule_from_isr(pointer item)
    {
        // TODO
    }

    process_result process_one(duration);
};


template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
class scheduler_with_event : public scheduler_base<Traits, Container>,
    scheduler_base2
{
    using base_type = scheduler_base<Traits, Container>;
    using event_group_type = estd::freertos::event_group<static_alloc>;

    event_group_type event_group_;

public:
    using typename base_type::duration;
    using typename base_type::time_point;

};

}

namespace layer1 {

template <ESTD_CPP_CONCEPT(embr::scheduler::concepts::Item) Item, unsigned N,
    ESTD_CPP_CONCEPT(embr::scheduler::concepts::Traits) Traits = embr::scheduler::item_traits<Item>>
using scheduler_with_notify =
    embr::scheduler::freertos::v1::detail::scheduler_with_notify<Traits, estd::layer1::vector<Item*, N>>;

template <ESTD_CPP_CONCEPT(embr::scheduler::concepts::Item) Item, unsigned N,
    ESTD_CPP_CONCEPT(embr::scheduler::concepts::Traits) Traits = embr::scheduler::item_traits<Item>>
using scheduler_with_event =
    embr::scheduler::freertos::v1::detail::scheduler_with_event<Traits, estd::layer1::vector<Item*, N>>;

}

}}}}


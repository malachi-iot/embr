#pragma once

#include <embr/scheduler.h>

namespace embr { namespace experimental {

template <class TTraits>
struct FreeRTOSSchedulerObserver
{
    typedef TTraits traits_type;

    void on_notify(embr::internal::events::Processing<traits_type>)
    {
    }

    void on_notify(embr::internal::events::Scheduling<traits_type>)
    {
    }

    void on_notify(embr::internal::events::Scheduled<traits_type>)
    {
    }

    void on_notify(embr::internal::events::Removed<traits_type>)
    {
    }
};

}}
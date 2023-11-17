#pragma once

#include <estd/type_traits.h>

#include "features.h"

#if FEATURE_EMBR_OBSERVER

namespace embr { namespace internal {


template <class Observer, class Event, class Context>
constexpr static bool notified_helper(const Observer&, Event, Context&, long)
{
    // TODO: Perhaps put our "didn't fire" warning here - compile time would be best
    return false;
}


template <class Observer, class Event, class Context>
constexpr static auto notified_helper(Observer&& observer, const Event& n, Context& context, bool)
    -> decltype(std::declval<Observer>().on_notified(n, context), bool{})
{
    return allow_notify_helper(observer, n, context, true) &&
        (observer.on_notified(n, context), true);
}

template <class Observer, class Event>
constexpr static auto notified_helper(Observer&& observer, const Event& n, estd::monostate, bool)
    -> decltype(std::declval<Observer>().on_notified(n), bool{})
{
    return allow_notify_helper(observer, n, true) &&
           (observer.on_notified(n), true);
}

}}

#endif
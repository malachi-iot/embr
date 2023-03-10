#pragma once

#include "features.h"

#if FEATURE_EMBR_OBSERVER

namespace embr { namespace internal {

template <class TObserver, class TEvent>
constexpr auto allow_notify_helper(TObserver& observer, const TEvent& n, long) -> bool
{
    return true;
}

template <class TObserver, class TEvent, class TContext>
constexpr auto allow_notify_helper(TObserver& observer, const TEvent& n, TContext&, long) -> bool
{
    return true;
}

template <class TObserver, class TEvent>
inline auto allow_notify_helper(TObserver& observer, const TEvent& n, bool)
    -> decltype(std::declval<TObserver>().allow_notify(n), bool{})
{
    return observer.allow_notify(n);
}

template <class TObserver, class TEvent, class TContext>
inline auto allow_notify_helper(TObserver& observer, const TEvent& n, TContext&, int)
    -> decltype(std::declval<TObserver>().allow_notify(n), bool{})
{
    return observer.allow_notify(n);
}

template <class TObserver, class TEvent, class TContext>
inline auto allow_notify_helper(TObserver& observer, const TEvent& n, TContext& context, bool)
    -> decltype(std::declval<TObserver>().allow_notify(n, context), bool{})
{
    return observer.allow_notify(n, context);
}


}}

#endif
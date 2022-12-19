#pragma once

#include "features.h"

#if FEATURE_EMBR_OBSERVER

namespace embr { namespace experimental {

template <class TObserver, class TEvent, class TContext>
inline static auto allow_notify_helper(TObserver& observer, const TEvent& n, long) -> bool
{
    return true;
}

template <class TObserver, class TEvent, class TContext>
inline static auto allow_notify_helper(TObserver& observer, const TEvent& n, TContext&, long) -> bool
{
    return true;
}

template <class TObserver, class TEvent, class TContext>
inline static auto allow_notify_helper(const TEvent& n, TContext& context, bool)
-> decltype(TObserver::allow_notify(n, context), bool{})
{
    return TObserver::allow_notify(n, context);
}

}}

#endif
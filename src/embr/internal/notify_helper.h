#pragma once

#include <estd/functional.h>

#include "features.h"
#include "allow_notify.h"

#if FEATURE_EMBR_OBSERVER

// DEBT: At this point, I think we always have a TObserver - so consider refactoring out
// the static-only flavors of all these helpers

namespace embr { namespace internal {

// FEATURE_EMBR_EXPLICIT_OBSERVER is helpful for enforcing < c++11 compliance or for
// troubleshooting lost notifications (due to improper overloading of on_notify)
#if defined(FEATURE_CPP_DECLTYPE) && !defined(FEATURE_EMBR_EXPLICIT_OBSERVER)
// https://stackoverflow.com/questions/23987925/using-sfinae-to-select-function-based-on-whether-a-particular-overload-of-a-func
// Used so that on_notify calls are optional
// fallback one for when we just can't match the on_notify
// Remember, trailing bool/int/long denotes priority with bool being best fit
template <class TObserver, class TEvent>
constexpr static auto notify_helper(TObserver& observer, const TEvent& n, long) -> bool
{
    // TODO: Perhaps put our "didn't fire" warning here - compile time would be best
    return true;
}


// fallback for invocation with context where no on_notify is present
template <class TObserver, class TEvent, class TContext>
constexpr static auto notify_helper(TObserver& observer, const TEvent& n, TContext&, long) -> bool
{
    // TODO: Perhaps put our "didn't fire" warning here - compile time would be best
    return true;
}

template <class TObserver, class TEvent>
constexpr static auto notify_helper(const TObserver&, const TEvent& n, long) -> bool
{
    static_assert(estd::is_const<const TObserver>::value == false,
                  "const not yet supported for notify_helper");

    return true;
}

template <class TObserver, class TEvent, class TContext>
constexpr static auto notify_helper(const TObserver&, const TEvent& n, TContext&, long) -> bool
{
    static_assert(estd::is_const<const TObserver>::value == false,
                  "const not yet supported for notify_helper");

    return true;
}

// bool gives this one precedence, since we call with (n, true)
template <class TObserver, class TEvent>
inline static auto notify_helper(TObserver& observer, const TEvent& e, bool)
    -> decltype(std::declval<TObserver>().on_notify(e), bool{})
{
    if(!allow_notify_helper(observer, e, true))
        return false;

    observer.on_notify(e);

    return true;
}

// pseudo-fallback to invoke non-context on_notify, even when context is present
template <class TObserver, class TEvent, class TContext>
inline static auto notify_helper(TObserver& observer, const TEvent& n, TContext& context, int)
    -> decltype(std::declval<TObserver>().on_notify(n), bool{})
{
    if(!allow_notify_helper(observer, n, context, true))
        return false;

    observer.on_notify(n);

    return true;
}

// bool gives this one precedence, since we call with (n, true)
template <class TObserver, class TEvent, class TContext>
inline static auto notify_helper(TObserver& observer, const TEvent& n, TContext& context, bool)
    -> decltype(std::declval<TObserver>().on_notify(n, context), bool{})
{
    if(!allow_notify_helper(observer, n, context, true))
        return false;

    observer.on_notify(n, context);

    return true;
}

#else
template <class TObserver, class TEvent>
inline static void notify_helper(TObserver& observer, const TEvent& n, bool)
{
    observer.on_notify(n);
}

template <class TObserver, class TEvent>
inline static void notify_helper(const TEvent& n, bool)
{
    TObserver::on_notify(n);
}
#endif

}}

#endif

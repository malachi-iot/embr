#pragma once

#include <estd/functional.h>

#include "features.h"
#include "allow_notify.h"

#if FEATURE_EMBR_OBSERVER

namespace embr { namespace internal {

// FEATURE_EMBR_EXPLICIT_OBSERVER is helpful for enforcing < c++11 compliance or for
// troubleshooting lost notifications (due to improper overloading of on_notify)
#if defined(FEATURE_CPP_DECLTYPE) && !defined(FEATURE_EMBR_EXPLICIT_OBSERVER)
// https://stackoverflow.com/questions/23987925/using-sfinae-to-select-function-based-on-whether-a-particular-overload-of-a-func
// Used so that on_notify calls are optional
// fallback one for when we just can't match the on_notify
// Remember, trailing bool/int/long denotes priority with bool being best fit
template <class TObserver, class TEvent>
constexpr static auto notify_helper(TObserver observer, const TEvent& n, long) -> bool
{
    // TODO: Perhaps put our "didn't fire" warning here - compile time would be best
    return true;
}


// fallback for invocation with context where no on_notify is present
template <class TObserver, class TEvent, class TContext>
constexpr static auto notify_helper(TObserver observer, const TEvent& n, TContext&, long) -> bool
{
    // TODO: Perhaps put our "didn't fire" warning here - compile time would be best
    return true;
}

// bool gives this one precedence, since we call with (n, true)
template <class TObserver, class TEvent>
inline static auto notify_helper(TObserver& observer, const TEvent& n, bool)
    -> decltype(std::declval<TObserver>().on_notify(n), void(), bool{})
{
    if(!experimental::allow_notify_helper<TObserver>(n, true))
        return false;

    observer.on_notify(n);

    return true;
}

// pseudo-fallback to invoke non-context on_notify, even when context is present
template <class TObserver, class TEvent, class TContext>
inline static auto notify_helper(TObserver& observer, const TEvent& n, TContext& context, int)
    -> decltype(std::declval<TObserver>().on_notify(n), void(), bool{})
{
    if(!experimental::allow_notify_helper<TObserver>(n, context, true))
        return false;

    observer.on_notify(n);

    return true;
}

// bool gives this one precedence, since we call with (n, true)
template <class TObserver, class TEvent, class TContext>
inline static auto notify_helper(TObserver& observer, const TEvent& n, TContext& context, bool)
    -> decltype(std::declval<TObserver>().on_notify(n, context), void(), bool{})
{
    if(!experimental::allow_notify_helper<TObserver>(n, context, true))
        return false;

    observer.on_notify(n, context);

    return true;
}

// stateless ones.  Probably we could use above ones but this way we can avoid
// inline construction of an entity altogether
// fallback one for when we just can't match the on_notify
template <class TObserver, class TEvent>
constexpr static auto notify_helper(const TEvent& n, long) -> bool
{
    return true;
}

// fallback for invocation with context where no on_notify is present
template <class TObserver, class TEvent, class TContext>
constexpr static auto notify_helper(const TEvent& n, TContext&, long) -> bool
{
    return true;
}

// bool gives this one precedence, since we call with (n, true)
template <class TObserver, class TEvent>
inline static auto notify_helper(const TEvent& n, bool)
    -> decltype(TObserver::on_notify(n), void(), bool{})
{
    if(!experimental::allow_notify_helper<TObserver>(n, true))
        return false;

    TObserver::on_notify(n);

    return true;
}


// bool gives this one precedence, since we call with (n, true)
template <class TObserver, class TEvent, class TContext>
inline static auto notify_helper(const TEvent& n, TContext& context, bool)
    -> decltype(TObserver::on_notify(n), void(), bool{})
{
    if(!experimental::allow_notify_helper<TObserver>(n, context, true))
        return false;

    TObserver::on_notify(n);

    return true;
}

// bool gives this one precedence, since we call with (n, true)
template <class TObserver, class TEvent, class TContext>
inline static auto notify_helper(const TEvent& n, TContext& context, bool)
    -> decltype(TObserver::on_notify(n, context), void(), bool{})
{
    if(!experimental::allow_notify_helper<TObserver>(n, context, true))
        return false;

    TObserver::on_notify(n, context);

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

#pragma once

#include <estd/exp/observer.h>
#include <estd/tuple.h>
#include <estd/functional.h>

#ifdef FEATURE_CPP_VARIADIC

// NOTE: Not otherwise labeled as such but this file's naming is experimental
namespace embr {

namespace internal {

// NOTE: yanked in from estd/exp/observer.h
// FEATURE_EMBR_EXPLICIT_OBSERVER is helpful for enforcing < c++11 compliance or for
// troubleshooting lost notifications (due to improper overloading of on_notify)
#if defined(FEATURE_CPP_DECLTYPE) && !defined(FEATURE_EMBR_EXPLICIT_OBSERVER)
// https://stackoverflow.com/questions/23987925/using-sfinae-to-select-function-based-on-whether-a-particular-overload-of-a-func
// Used so that on_notify calls are optional
// fallback one for when we just can't match the on_notify
template <class TObserver, class TNotifier>
static auto notify_helper(TObserver& observer, const TNotifier& n, int) -> bool
{
    return true;
}


// fallback for invocation with context where no on_notify is present
template <class TObserver, class TNotifier, class TContext>
static auto notify_helper(TObserver& observer, const TNotifier& n, TContext&, int) -> bool
{
    return true;
}

// bool gives this one precedence, since we call with (n, true)
template <class TObserver, class TNotifier>
static auto notify_helper(TObserver& observer, const TNotifier& n, bool)
-> decltype(std::declval<TObserver>().on_notify(n), void(), bool{})
{
    observer.on_notify(n);

    return true;
}

template <class TObserver, class TNotifier, class TContext>
static auto notify_helper(TObserver& observer, const TNotifier& n, TContext& context, bool)
-> decltype(std::declval<TObserver>().on_notify(n), void(), bool{})
{
    observer.on_notify(n);

    return true;
}

// bool gives this one precedence, since we call with (n, true)
template <class TObserver, class TNotifier, class TContext>
static auto notify_helper(TObserver& observer, const TNotifier& n, TContext& context, bool)
-> decltype(std::declval<TObserver>().on_notify(n, context), void(), bool{})
{
    observer.on_notify(n, context);

    return true;
}

// stateless ones.  Probably we could use above ones but this way we can avoid
// inline construction of an entity altogether
// fallback one for when we just can't match the on_notify
template <class TObserver, class TNotifier>
static auto notify_helper(const TNotifier& n, int) -> bool
{
    return true;
}

// fallback for invocation with context where no on_notify is present
template <class TObserver, class TNotifier, class TContext>
static auto notify_helper(const TNotifier& n, TContext&, int) -> bool
{
    return true;
}

// bool gives this one precedence, since we call with (n, true)
template <class TObserver, class TNotifier>
static auto notify_helper(const TNotifier& n, bool)
-> decltype(TObserver::on_notify(n), void(), bool{})
{
    TObserver::on_notify(n);

    return true;
}


// bool gives this one precedence, since we call with (n, true)
template <class TObserver, class TNotifier, class TContext>
static auto notify_helper(const TNotifier& n, TContext& context, bool)
-> decltype(TObserver::on_notify(n), void(), bool{})
{
    TObserver::on_notify(n);

    return true;
}

// bool gives this one precedence, since we call with (n, true)
template <class TObserver, class TNotifier, class TContext>
static auto notify_helper(const TNotifier& n, TContext& context, bool)
-> decltype(TObserver::on_notify(n, context), void(), bool{})
{
    TObserver::on_notify(n, context);

    return true;
}
#else
template <class TObserver, class TNotifier>
static void notify_helper(TObserver& observer, const TNotifier& n, bool)
{
    observer.on_notify(n);
}

template <class TObserver, class TNotifier>
static void notify_helper(const TNotifier& n, bool)
{
    TObserver::on_notify(n);
}
#endif

template <class ...TObservers>
class tuple_base
{
protected:
    typedef estd::tuple<TObservers...> tuple_type;

    tuple_type observers;

    template <int index, class TEvent>
    void _notify_helper(const TEvent& e)
    {
        estd::tuple_element_t<index, tuple_type>& observer =
                estd::get<index>(observers);

        estd::experimental::internal::notify_helper(observer, e, true);
    }

    template <int index, class TEvent, class TContext>
    void _notify_helper(const TEvent& e, TContext& c)
    {
        estd::tuple_element_t<index, tuple_type>& observer =
                estd::get<index>(observers);

        estd::experimental::internal::notify_helper(observer, e, c, true);
    }

    tuple_base(TObservers&&...observers) :
        observers(std::forward<TObservers>(observers)...)
    {}

    tuple_base() {}

};

// slightly abuses tuple type.  We pretend we have a tuple
template <class ...TObservers>
class stateless_base
{
protected:
    typedef estd::tuple<TObservers...> tuple_type;

    template <int index, class TEvent>
    void _notify_helper(const TEvent& e)
    {
        // allocate a purely temporary observer, as this has
        // been explicitly set up as stateless
        estd::tuple_element_t<index, tuple_type> observer;

        // SFINAE magic to call best matching on_notify function
        estd::experimental::internal::notify_helper(
                    observer,
                    e, true);
    }
};

template <class TBase, class ...TObservers>
class subject : TBase
{
    typedef TBase base_type;
    typedef typename base_type::tuple_type tuple_type;

    // using slightly clumsy index >= 0 so that Qt tabbing doesn't get confused
    template <int index, class TEvent,
              class TEnabled = typename estd::enable_if <!(index >= 0), bool>::type >
    void notify_helper(const TEvent&) const
    {

    }

    template <int index, class TEvent, class TContext,
              class TEnabled = typename estd::enable_if <!(index >= 0), bool>::type >
    void notify_c_helper(const TEvent&, TContext&) const
    {

    }

    template <int index, class TEvent,
              class TEnabled = typename estd::enable_if<(index >= 0), void>::type >
    void notify_helper(const TEvent& e, bool = true)
    {
        base_type::template _notify_helper<index>(e);

        notify_helper<index - 1>(e);
    }

    template <int index, class TEvent, class TContext,
              class TEnabled = typename estd::enable_if<(index >= 0), void>::type >
    void notify_c_helper(const TEvent& e, TContext& c, bool = true)
    {
        base_type::template _notify_helper<index>(e, c);

        notify_c_helper<index - 1>(e, c);
    }
public:
    constexpr subject(TObservers&&...observers) :
            base_type(std::forward<TObservers>(observers)...)
    {}

    subject() {}

    template <class TEvent>
    void notify(const TEvent& e)
    {
        notify_helper<sizeof... (TObservers) - 1>(e);
    }

    template <class TEvent, class TContext>
    void notify(const TEvent& e, TContext& c)
    {
        notify_c_helper<sizeof... (TObservers) - 1>(e, c);
    }
};

}

namespace layer0 {

#ifdef FEATURE_CPP_ALIASTEMPLATE
template <class ...TObservers>
using subject = internal::subject<
    internal::stateless_base<TObservers...>,
    TObservers...>;

#endif

}
namespace layer1 {

#ifdef FEATURE_CPP_ALIASTEMPLATE
template <class ...TObservers>
using subject = internal::subject<
    internal::tuple_base<TObservers...>,
    TObservers...>;
#else
template <class ...TObservers>
class subject : internal::subject<internal::tuple_base<TObservers...> >
{
public:
};
#endif

template <class ...TObservers>
subject<TObservers&&...> make_subject(TObservers&&...observers)
{
    return subject<TObservers&&...>(
            std::forward<TObservers>(observers)...);
}



}

struct void_subject
{
    template <class TNotifier>
    void notify(const TNotifier&) {}

    template <class TNotifier, class TContext>
    void notify(const TNotifier&, TContext&) {}
};



}

#endif


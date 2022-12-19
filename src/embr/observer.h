/**
 * @file
 * @author Malachi Burke
 */
#pragma once

#include <estd/functional.h>
#include "internal/notify_helper.h"
#include "internal/argtype.h"
#include "internal/subject.h"


namespace embr {

#if FEATURE_EMBR_OBSERVER

namespace layer0 {

#ifdef FEATURE_CPP_ALIASTEMPLATE
template <class ...TObservers>
using subject = internal::subject<
    internal::stateless_base<TObservers...>,
    TObservers...>;

#endif

// TODO: Make two varieties of observer_proxy:
// 1.  Fully stateless subject wrapper
// 2.  'Global' version binding to an existing global/static subject

namespace experimental {

// stateless variety
template <class TSubject>
class observer_proxy
{
    template <class TEvent>
    static void on_notify(const TEvent& e)
    {
        TSubject().notify(e);
    }

    template <class TEvent, class TContext>
    static void on_notify(const TEvent& n, TContext& c)
    {
        TSubject().notify(n, c);
    }
};

}

}

namespace experimental {

template <typename F, class TEvent>
struct delegate_observer
{
    F f;

    delegate_observer(F&& f) : f(std::move(f)) {}

    // FIX: Needs harder event type here otherwise we get cross-wiring on argument types
    // when calling f
    // see
    void on_notify(const TEvent& e)
    {
        f(e);
    }
};

template <typename F, typename Arg1 = typename internal::ArgType<decltype(internal::ArgHelper(&F::operator())) >::arg1 >
struct delegate_observer<F, Arg1> make_delegate_observer(F&& f)
{
    return delegate_observer<F, Arg1>(std::move(f));
}

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


namespace internal {

// presents itself as an observer but is in fact a wrapper around a
// subject reference
template <class TSubject>
class observer_proxy
{
    TSubject& subject;

public:
    /// @brief pass on event to underlying subject to re-broadcast
    /// \tparam TEvent
    /// \param e
    template <class TEvent>
    void on_notify(const TEvent& e)
    {
        subject.notify(e);
    }

    template <class TEvent, class TContext>
    void on_notify(const TEvent& n, TContext& c)
    {
        subject.notify(n, c);
    }

    observer_proxy(TSubject& s) : subject(s) {}

    // Proxy by nature operates on a TSubject reference, not a value - so no
    // move semantic version
#ifdef FEATURE_CPP_MOVESEMANTIC
    //observer_proxy(TSubject&& s) : subject(std::move(s)) {}
#endif
};

}

// TODO: Move observer_proxy out of internal altogether
#ifdef FEATURE_CPP_ALIASTEMPLATE
template <class TSubject>
using observer_proxy = internal::observer_proxy<TSubject>;
#endif


template <class TSubject>
internal::observer_proxy<TSubject> make_observer_proxy(TSubject& s)
{
    return internal::observer_proxy<TSubject>(s);
}


}

#endif

struct void_subject
{
    /// @brief noop notify
    /// \tparam TEvent
    template <class TEvent>
    void notify(const TEvent&) const {}

    /// @brief noop notify
    /// \tparam TEvent
    /// \tparam TContext
    template <class TEvent, class TContext>
    void notify(const TEvent&, TContext&) const {}
};



}


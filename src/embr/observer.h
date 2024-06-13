/**
 * @file
 * @author Malachi Burke
 */
#pragma once

#include <estd/functional.h>
#include "internal/notify_helper.h"
#include "internal/argtype.h"
#include "internal/subject.h"
#include "fwd/observer.h"


namespace embr {

#if FEATURE_EMBR_OBSERVER

namespace layer0 {

#ifdef __cpp_alias_templates
template <class ...TObservers>
using subject = internal::subject<
    internal::stateless_base<TObservers...> >;

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

    constexpr explicit delegate_observer(F&& f) : f(std::move(f)) {}

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
    return delegate_observer<F, Arg1>(std::forward<F>(f));
}


}

namespace layer1 {

template <class ...Observers>
using subject = internal::subject<
    internal::tuple_base<Observers...> >;

template <class ...Observers>
constexpr subject<Observers...> make_subject(Observers&&...observers)
{
    return subject<Observers...>(
            std::forward<Observers>(observers)...);
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

    constexpr explicit observer_proxy(TSubject& s) : subject(s) {}

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

namespace experimental {

template <class ...Tuples>
struct tuple_cat_helper;

template <class ...Types>
struct tuple_retrieve_helper;

template <class T, class ...Types>
struct tuple_retrieve_helper<T, Types...> {};

template <class ...Types, class ...Types2>
struct tuple_cat_helper<estd::tuple<Types...>, estd::tuple<Types2...> >
{
    using type = estd::tuple<Types..., Types2...>;

    static type tester(estd::tuple<Types...>& t1, estd::tuple<Types2...>& t2)
    {
        return estd::apply([&](Types&&... args1)
        {
            return estd::apply([&](Types2&&...args2)
            {
                return type(
                    std::forward<Types>(args1)...,
                    std::forward<Types2>(args2)...);
            }, t2);
        }, t1);
    }
};


//template <class ...Tuples>
//estd::tuple<

template <class Observer, class ...Observers>
auto append(layer0::subject<Observers...>, Observer& o) -> layer1::subject<Observers..., Observer>
{
    using type = layer1::subject<Observers..., Observer>;

    return type{};
}

}

inline namespace v1 {

// This almost handles both layer0 and layer1 - though layer0 may morph
// into something that enforces 0-size.  Also, only layer0
// has 'append' capability

template <class ...Observers>
using subject = layer1::subject<Observers...>;

}

#endif


}


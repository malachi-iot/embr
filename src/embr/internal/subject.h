#pragma once

#include <estd/tuple.h>

#include "notify_helper.h"

#if FEATURE_EMBR_OBSERVER

namespace embr { namespace internal {

template <class ...TObservers>
class tuple_base
{
protected:
    typedef estd::tuple<TObservers...> tuple_type;

    tuple_type observers;

    template <int index, class TEvent>
    void _notify_helper(const TEvent& e)
    {
        notify_helper(get<index>(), e, true);
    }

    template <int index, class TEvent, class TContext>
    void _notify_helper(const TEvent& e, TContext& c)
    {
        notify_helper(get<index>(), e, c, true);
    }

    tuple_base(TObservers&&...observers) :
        observers(std::forward<TObservers>(observers)...)
    {}

    tuple_base() {}

public:
    // TODO: Move these gets out into a tuple base/wrapper
    // NOTE: Consider also making this into a tuple() call
    template <int index>
    estd::tuple_element_t<index, tuple_type >& get()
    {
        return estd::get<index>(observers);
    }

    template <int index>
    const estd::tuple_element_t<index, tuple_type >& get() const
    {
        return estd::get<index>(observers);
    }
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
        notify_helper(
                    observer,
                    e, true);
    }

    template <int index, class TEvent, class TContext>
    void _notify_helper(const TEvent& e, TContext& c)
    {
        // allocate a purely temporary observer, as this has
        // been explicitly set up as stateless
        estd::tuple_element_t<index, tuple_type> observer;

        // SFINAE magic to call best matching on_notify function
        notify_helper(
                    observer,
                    e, c, true);
    }
};

template <class TBase, class ...TObservers>
class subject : public TBase
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
        notify_helper<index - 1>(e);

        base_type::template _notify_helper<index>(e);
    }

    template <int index, class TEvent, class TContext,
              class TEnabled = typename estd::enable_if<(index >= 0), void>::type >
    void notify_c_helper(const TEvent& e, TContext& c, bool = true)
    {
        notify_c_helper<index - 1>(e, c);

        base_type::template _notify_helper<index>(e, c);
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


}}

#endif

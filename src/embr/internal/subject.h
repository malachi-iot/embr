#pragma once

#include <estd/tuple.h>

#include "notify_helper.h"
#include "fwd.h"

#if FEATURE_EMBR_OBSERVER

namespace embr { namespace internal {

// Designed to hang off visit_tuple_functor results
// DEBT: Not used yet
class subject_visitor
{
    template <size_t I, class T, class Event, class Context>
    bool operator()(estd::variadic::instance<I, T> i, Event& e) const
    {
        return notify_helper(i.value, e, true);
    }

    template <size_t I, class T, class Event, class Context>
    bool operator()(estd::variadic::instance<I, T> i, Event& e, Context& c) const
    {
        return notify_helper(i.value, e, c, true);
    }
};

template <class ...TObservers>
class tuple_base
{
protected:
    typedef estd::tuple<TObservers...> tuple_type;

    // In case of stateless types, we want tuple to help us select a value or a reference
    template <std::size_t index>
    using valref_type = typename estd::tuple_element<index, tuple_type>::valref_type;

    // DEBT: use an evaporator here so that full sparse/stateless take 0 bytes instead of 1
    tuple_type observers;

    template <int index, class TEvent>
    void _notify_helper(const TEvent& e)
    {
        valref_type<index> observer = estd::get<index>(observers);

        notify_helper(observer, e, true);
    }

    template <int index, class TEvent, class TContext>
    void _notify_helper(const TEvent& e, TContext& c)
    {
        valref_type<index> observer = estd::get<index>(observers);

        notify_helper(observer, e, c, true);
    }

    constexpr explicit tuple_base(TObservers&&...observers) :
        observers(std::forward<TObservers>(observers)...)
    {}

    tuple_base() = default;

public:
    // TODO: Move these gets out into a tuple base/wrapper
    // NOTE: Consider also making this into a tuple() call
    template <std::size_t index>
    valref_type<index> get()
    {
        return estd::get<index>(observers);
    }

    template <int index>
    typename estd::tuple_element<index, tuple_type >::const_valref_type get() const
    {
        return estd::get<index>(observers);
    }
};


namespace tag {

struct stateless_subject {};

}


// slightly abuses tuple type.  We pretend we have a tuple
template <class ...TObservers>
class stateless_base : tag::stateless_subject
{
protected:
    typedef estd::tuple<TObservers...> tuple_type;

    // allocate a purely temporary observer, as this has
    // been explicitly set up as stateless
    template <class T>
    struct provider : estd::type_identity<T>
    {
        constexpr static T value()
        {
            static_assert(estd::is_empty<T>::value, "T must be stateless");
            return T();
        }
    };

    template <class T, T* t>
    struct provider<estd::integral_constant<T*, t> >
    {
        typedef T& type;

        static constexpr type value()
        {
            return *t;
        }
    };

#ifdef FEATURE_STD_TYPE_TRAITS
    // UNTESTED
    template <class T, T t>
    struct provider<std::integral_constant<T, t> >
    {
        typedef typename estd::remove_pointer<T>::type& type;

        static type value() { return *t; }
    };
#endif

    template <int index>
    using p = provider<estd::tuple_element_t<index, tuple_type> >;

    template <int index, class TEvent>
    void _notify_helper(const TEvent& e)
    {
        typename p<index>::type observer = p<index>::value();

        static_assert(estd::is_const<typename p<index>::type>::value == false,
            "const not yet supported for notify_helper");

        // SFINAE magic to call best matching on_notify function
        notify_helper(
                    observer,
                    e, true);
    }

    template <int index, class TEvent, class TContext>
    void _notify_helper(const TEvent& e, TContext& c)
    {
        typename p<index>::type observer = p<index>::value();

        // SFINAE magic to call best matching on_notify function
        notify_helper(
                    observer,
                    e, c, true);
    }

public:
    // DEBT: Don't love using the consuming 'subject' directly here, but putting this alias
    // in 'subject' itself presents an issue for layer1 scenarios
    template <class ...TObservers2>
    using append = subject<stateless_base<TObservers..., TObservers2...>>;
};

template <class TBase>
class subject : public TBase
{
    typedef TBase base_type;
    typedef typename base_type::tuple_type tuple_type;

    static constexpr size_t size()
    {
        return estd::tuple_size<tuple_type>::value;
    }


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
    ESTD_CPP_FORWARDING_CTOR(subject)
    ESTD_CPP_DEFAULT_CTOR(subject)

    template <class TEvent>
    void notify(const TEvent& e)
    {
        notify_helper<size() - 1>(e);
    }

    template <class TEvent, class TContext>
    void notify(const TEvent& e, TContext& c)
    {
        notify_c_helper<size() - 1>(e, c);
    }
};


}}

#endif

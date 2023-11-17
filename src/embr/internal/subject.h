#pragma once

#include <estd/tuple.h>

#include "notify_helper.h"
#include "observer/notified.h"
#include "fwd.h"

#if FEATURE_EMBR_OBSERVER

namespace embr { namespace internal {

template <class ...TObservers>
class tuple_base;


namespace tag {

struct stateless_subject {};

}

// TODO: 'provider' works well enough we might be able to consolidate
// layer0/layer1 behavior into provider itself, while tuple_base's
// sparse_tuple makes it a candidate for stateless_base to derive from
// NOTE: 'Types' is experimental here so far
template <bool stateless, class Types>
struct providers
{
    template <class T>
    using is_empty = estd::enable_if_t<estd::is_empty<T>::value>;

    template <class T>
    struct is_integral_constant : estd::false_type {};

    template <class T, T* t>
    struct is_integral_constant<estd::integral_constant<T*, t> > : estd::true_type {};

    template <class T, class = void>
    struct provider;

    template <class T>
    struct provider<T,
        estd::enable_if_t<
            !estd::is_empty<T>::value &&
            !is_integral_constant<T>::value>> :
        estd::type_identity<T>
    {
        constexpr static T& value(T& v)
        {
            static_assert(!stateless, "T must be stateless");
            return v;
        }
    };

    // allocate a purely temporary observer, as this has
    // been explicitly set up as stateless
    template <class T>
    struct provider<T,
        estd::enable_if_t<
            estd::is_empty<T>::value &&
            !is_integral_constant<T>::value>> :
        estd::type_identity<T>
    {
        constexpr static T value(T&&)
        {
            return T();
        }
    };

    template <class T>
    struct provider<T,
        estd::enable_if_t<is_integral_constant<T>::value>>
    {
        typedef typename estd::remove_pointer_t<typename T::value_type>& type;

        static constexpr type value(T&&)
        {
            return *T::value;
        }
    };

#ifdef FEATURE_STD_TYPE_TRAITSX
    // UNTESTED
    template <class T, T* t>
    struct provider<std::integral_constant<T*, t>, is_empty<T>>
    {
        typedef T& type;

        static constexpr type value() { return *t; }
    };
#endif
};



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
class tuple_base : public estd::tuple<TObservers...>
{
protected:
    using types = estd::variadic::types<TObservers...>;

public:
    using tuple_type = estd::tuple<TObservers...>;

    tuple_type& observers() { return *this; }
    const tuple_type& observers() const { return *this; }

protected:
    // In case of stateless types, we want tuple to help us select a value or a reference
    template <std::size_t index>
    using valref_type = typename estd::tuple_element<index, tuple_type>::valref_type;

    template <int index, class TEvent>
    bool _notify_helper(const TEvent& e)
    {
        valref_type<index> observer = estd::get<index>(observers());

        return notify_helper(observer, e, true);
    }

    template <int index, class TEvent, class TContext>
    bool _notify_helper(const TEvent& e, TContext& c)
    {
        valref_type<index> observer = estd::get<index>(observers());

        return notify_helper(observer, e, c, true);
    }

    template <int index, class Event, class Context>
    constexpr bool _notified_helper(const Event& e, Context& c)
    {
        valref_type<index> observer = estd::get<index>(observers());

        return notified_helper(observer, e, c, true);
    }

    constexpr explicit tuple_base(TObservers&&...observers) :
        tuple_type(std::forward<TObservers>(observers)...)
    {}

    tuple_base() = default;
};

template <class ...TObservers>
class stateless_base : tag::stateless_subject
{
protected:
    using types = estd::variadic::types<TObservers...>;

    template <size_t index>
    using type_at_index = typename types::template get<index>;

    template <int index>
    using p = typename providers<true, types>::template provider<type_at_index<index> >;

    template <int index, class TEvent>
    static constexpr bool _notify_helper(const TEvent& e)
    {
        using type = type_at_index<index>;
        //using observer_type = typename p<index>::type;

        static_assert(estd::is_empty<type>::value, "layer0 demands empty type");

        // SFINAE magic to call best matching on_notify function
        return notify_helper(
                    p<index>::value(type{}),
                    e, true);
    }

    template <int index, class TEvent, class TContext>
    static constexpr bool _notify_helper(const TEvent& e, TContext& c)
    {
        using type = type_at_index<index>;
        //using observer_type = typename p<index>::type;

        static_assert(estd::is_empty<type>::value, "layer0 demands empty type");

        // SFINAE magic to call best matching on_notify function
        return notify_helper(
                    p<index>::value(type{}),
                    e, c, true);
    }

    template <int index, class Event, class Context>
    static constexpr bool _notified_helper(const Event& e, Context& c)
    {
        using type = type_at_index<index>;
        //using observer_type = typename p<index>::type;

        static_assert(estd::is_empty<type>::value, "layer0 demands empty type");

        // SFINAE magic to call best matching on_notify function
        return notified_helper(
            p<index>::value(type{}),
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
    using visitor = typename base_type::types::visitor;

    struct notifying_functor
    {
        template <size_t I, class T, class Event>
        constexpr bool operator()(estd::variadic::type<I, T>, subject& this_, Event& e) const
        {
            return (this_.template _notify_helper<I>(e), false);
        }

        template <size_t I, class T, class Event, class Context>
        constexpr bool operator()(estd::variadic::type<I, T>, subject& this_, Event& e, Context& c) const
        {
            return (this_.template _notify_helper<I>(e, c), false);
        }
    };

    struct notified_functor
    {
        template <size_t I, class T, class Event, class Context>
        constexpr bool operator()(estd::variadic::type<I, T>, subject& this_, Event& e, Context& c) const
        {
            return (this_.template _notified_helper<I>(e, c), false);
        }

        template <size_t I, class T, class Event, class Context>
        constexpr bool operator()(estd::variadic::type<I, T>, subject& this_, Event& e, const Context& c) const
        {
            return (this_.template _notified_helper<I>(e, c), false);
        }
    };

public:
    ESTD_CPP_FORWARDING_CTOR(subject)
    ESTD_CPP_DEFAULT_CTOR(subject)

    template <class Event>
    void notify(const Event& e)
    {
        visitor::visit(notifying_functor{}, *this, e);
        visitor::visit_reverse(notified_functor{}, *this, e, estd::monostate{});
    }

    template <class Event, class Context>
    void notify(const Event& e, Context& c)
    {
        visitor::visit(notifying_functor{}, *this, e, c);
        visitor::visit_reverse(notified_functor{}, *this, e, c);
    }

    template <class Event, class Context>
    void notify(const Event& e, Context& c) const
    {
        visitor::visit(notifying_functor{}, *this, e, c);
        visitor::visit_reverse(notified_functor{}, *this, e, c);
    }
};


}}

#endif

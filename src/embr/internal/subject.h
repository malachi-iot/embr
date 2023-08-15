#pragma once

#include <estd/tuple.h>

#include "notify_helper.h"
#include "fwd.h"

#if FEATURE_EMBR_OBSERVER

namespace embr { namespace internal {

namespace tag {

struct stateless_subject {};

}


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
    void _notify_helper(const TEvent& e)
    {
        valref_type<index> observer = estd::get<index>(observers());

        notify_helper(observer, e, true);
    }

    template <int index, class TEvent, class TContext>
    void _notify_helper(const TEvent& e, TContext& c)
    {
        valref_type<index> observer = estd::get<index>(observers());

        notify_helper(observer, e, c, true);
    }

    constexpr explicit tuple_base(TObservers&&...observers) :
        tuple_type(std::forward<TObservers>(observers)...)
    {}

    tuple_base() = default;
};

// TODO: 'provider' works well enough we might be able to consolidate
// layer0/layer1 behavior into provider itself, while tuple_base's
// sparse_tuple makes it a candidate for stateless_base to derive from
struct stateless_providers
{
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
    template <class T, T* t>
    struct provider<std::integral_constant<T*, t> >
    {
        typedef T& type;

        static constexpr type value() { return *t; }
    };
#endif
};


template <class ...TObservers>
class stateless_base : tag::stateless_subject
{
protected:
    using types = estd::variadic::types<TObservers...>;

    template <size_t index>
    using type_at_index = typename types::template get<index>;

    template <int index>
    using p = stateless_providers::provider<type_at_index<index> >;

    template <int index, class TEvent>
    void _notify_helper(const TEvent& e)
    {
        using observer_type = typename p<index>::type;
        observer_type observer = p<index>::value();

        static_assert(estd::is_empty<type_at_index<index>>::value, "layer0 demands empty type");
        static_assert(estd::is_const<observer_type>::value == false,
            "const not yet supported for notify_helper");

        // SFINAE magic to call best matching on_notify function
        notify_helper(
                    observer,
                    e, true);
    }

    template <int index, class TEvent, class TContext>
    void _notify_helper(const TEvent& e, TContext& c)
    {
        using observer_type = typename p<index>::type;
        observer_type observer = p<index>::value();

        static_assert(estd::is_empty<type_at_index<index>>::value, "layer0 demands empty type");

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
    using visitor = typename base_type::types::visitor;

    struct functor
    {
        subject& this_;

        template <size_t I, class T, class Event>
        bool operator()(estd::variadic::type<I, T>, Event& e) const
        {
            this_.template _notify_helper<I>(e);
            return false;
        }

        template <size_t I, class T, class Event, class Context>
        bool operator()(estd::variadic::type<I, T>, Event& e, Context& c) const
        {
            this_.template _notify_helper<I>(e, c);
            return false;
        }
    };

public:
    ESTD_CPP_FORWARDING_CTOR(subject)
    ESTD_CPP_DEFAULT_CTOR(subject)

    template <class TEvent>
    void notify(const TEvent& e)
    {
        visitor::visit(functor{*this}, e);
    }

    template <class TEvent, class TContext>
    void notify(const TEvent& e, TContext& c)
    {
        visitor::visit(functor{*this}, e, c);
    }
};


}}

#endif

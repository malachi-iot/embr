#pragma once

#include <estd/tuple.h>

#include "notify_helper.h"
#include "fwd.h"

#if FEATURE_EMBR_OBSERVER

namespace embr { namespace internal {

template <class ...TObservers>
class tuple_base
{
protected:
    typedef estd::tuple<TObservers...> tuple_type;

    // In case of stateless types, we want tuple to help us select a value or a reference
    template <std::size_t index>
    using valref_type = typename estd::tuple_element<index, tuple_type>::valref_type;

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

// EXPERIMENTAL
// Kind of our own integral constant.  Not so useful as it is
template <class T, T* t>
struct static_wrapper
{
    T& value() { return *t; }
    operator T&() { return *t; }
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
    struct provider
    {
        constexpr static T value()
        {
            static_assert(estd::is_empty<T>::value, "T must be stateless");
            return T();
        }

        typedef T type;
    };

    // Grab a global static wrapped up as a type, oftentimes a singleton
    template <class T, T* t>
    struct provider<static_wrapper<T, t> >
    {
        constexpr static T& value() { return *t; }
        typedef T& type;
    };

    template <class T, T t>
    struct provider<estd::integral_constant<T, t> >
    {
        // DEBT: estd doesn't have remove_pointer just yet
        typedef typename estd::remove_pointer<T>::type& type;

        static constexpr type value()
        {
            /*
            T _t = estd::integral_constant<T, t>();
            return *_t; */
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
    using append = subject<stateless_base<TObservers..., TObservers2...>, TObservers..., TObservers2...>;
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

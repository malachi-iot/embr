#pragma once

#include <bitset>
#include <estd/internal/variadic.h>

#include <embr/service.h>

// Service auto starter

// NOTE: Doing this with types directly, but IDs may be better to
// resolve cross dependencies

namespace embr { namespace experimental {

// For a fully ref'd out subject
template <class ...Observers>
class tupleref_base
{
protected:
    using types = estd::variadic::types<Observers...>;

public:
    using tuple_type = estd::tuple<Observers...>;

protected:
    tuple_type& tuple;

public:
    tuple_type& observers() { return tuple; }
    const tuple_type& observers() const { return tuple; }

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

public:
    constexpr explicit tupleref_base(tuple_type& tuple) : tuple{tuple}
    {}
};

// Designed to be used on tuple::visit of all services
template <class Service>
struct service_starter_functor
{
    Service& service;
    int& counter;

    template <size_t I, class T, size_t S, class ...Types>
    bool operator()(estd::variadic::instance<I, T> vi, estd::tuple<Types...>& tuple, std::bitset<S>& b)
    {
        using depends_on = typename T::depends_on;

        ++counter;

        tupleref_base<Types...> subject(tuple);

        if(b.test(I)) return false;

        b.set(I);

        // Recursively scan 'tuple' for depends_on
        tuple.visit(service_starter_functor<T>{vi.value, counter}, tuple, b);

        // revisiting the old is-a vs has-a impl() for services situation
        //T& service = vi.value;

        //service.start();

        return false;
    }
};

}}

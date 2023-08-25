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
    tuple_type* tuple;

public:
    tuple_type& observers() { return *tuple; }
    const tuple_type& observers() const { return *tuple; }

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
    template <class ...Observers2>
    void assign_tuple(estd::tuple<Observers2...>& other)
    {
        // NOTE: A bit of black magic here, document better once concept is proven
        tuple = (tuple_type*) &other;
    }

    tupleref_base() = default;

    constexpr explicit tupleref_base(tuple_type& tuple) : tuple{&tuple}
    {}
};

template <class ...Observers>
using ref_subject = internal::subject<
    tupleref_base<Observers...> >;

template <class T>
struct is_same_impl_selector
{
    template <class T_j, size_t J>
    using evaluator = estd::is_same<typename T_j::service_core_exp_type, T>;
};


// Grabs anything out of list whose impl matches up to specified runtime
template <class T, class ...Args>
using select_impl = estd::variadic::selector<is_same_impl_selector<T>, Args...>;


template <class Runtime, class Tuple>
void do_autostart(Runtime& runtime, Tuple& tuple)
{
    // revisiting the old is-a vs has-a impl() for services situation
    // Black magic - we're assuming service runtime has a ref_subject<void> and since that
    // occupies identical memory space as any other ref_subject, we recast it to
    // ref_subject containing all the observers we're interested in
    runtime.assign_tuple(tuple);

    // Black magic reassign current "listening" observers.  Things to note:
    // 1. really only needs to be assigned once, but figuring that out is a little tricky
    // 2. NOT thread safe (in theory) but in reality since we're (almost?) always reassigning
    //    to the same value, it is
    runtime.start();
}


// Designed to be used on tuple::visit of all services
template <class Service>
struct service_starter_functor
{
    Service& service;
    int& counter;

    template <size_t I, class T, size_t S, class ...Types>
    bool operator()(estd::variadic::v1::visitor_index<I, T> t, estd::tuple<Types...>& tuple, std::bitset<S>& b) const
    {
        using single = typename select_impl<T, Types...>::single;
        using impl_type = typename single::type::service_core_exp_type;
        constexpr size_t index = single::index;

        if(b.test(index)) return false;

        b.set(index);

        using subject_type = ref_subject<Types...>;

        using runtime = typename T::template runtime<subject_type, impl_type>;

        do_autostart((runtime&)estd::get<index>(tuple), tuple);

        return false;
    }

    template <size_t I, class T, size_t S, class ...Types>
    bool operator()(estd::variadic::instance<I, T> vi, estd::tuple<Types...>& tuple, std::bitset<S>& b)
    {
        using depends_on = typename T::depends_on;
        using impl_type = typename T::service_core_exp_type;

        depends_on::visitor::visit(*this, tuple, b);
        //depends_on::visitor::visit(service_starter_functor{service});

        ++counter;

        using subject_type = ref_subject<Types...>;

        if(b.test(I)) return false;

        b.set(I);

        // Recursively scan 'tuple' for depends_on
        tuple.visit(service_starter_functor<T>{vi.value, counter}, tuple, b);

        using runtime = typename T::template runtime<subject_type, impl_type>;

        do_autostart((runtime&)vi.value, tuple);

        return false;
    }
};

}}

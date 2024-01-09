#pragma once

#include <bitset>
#include <estd/internal/variadic.h>

#include <embr/service.h>

#include "refsubject.h"

// Service auto starter

// NOTE: Doing this with types directly, but IDs may be better to
// resolve cross dependencies

namespace embr { namespace experimental {




// Grabs anything out of list whose impl matches up to specified runtime
template <class T, class ...Args>
using select_impl = estd::variadic::v1::selector<is_same_impl_selector<T>, Args...>;


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

// NOTE: Doesn't help.  Just keeping here so we know that
template <class T, class Subject, class Impl>
struct runtime_access_workaround : T
{
    using base_type = T;
    using runtime2 = typename base_type::template runtime<Subject, Impl>;
};


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

        // DEBT: This is generating warnings
        using runtime = typename T::template runtime<subject_type, impl_type>;
        //using runtime = typename runtime_access_workaround<T, subject_type, impl_type>::runtime2;

        // TODO: Look out, casting to reference is one of those few areas
        // that isn't quite interchangeable with pointer behavior.  Consider
        // breaking out to a teporary variable
        do_autostart((runtime&)vi.value, tuple);

        return false;
    }
};

}}

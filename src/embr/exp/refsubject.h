#pragma once

#include <bitset>
#include <estd/internal/variadic.h>

#include <embr/service.h>

namespace embr { namespace experimental {

// For a fully ref'd out subject
// DEBT: High maintenance since it duplicates existing subject code
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

    template <int index, class TEvent, class TContext>
    bool _notified_helper(const TEvent& e, TContext& c)
    {
        valref_type<index> observer = estd::get<index>(observers());

        return notified_helper(observer, e, c, true);
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

template <class T>
struct is_same_impl_selector
{
    template <class T_j, size_t J>
    using evaluator = estd::is_same<typename T_j::service_core_exp_type, T>;
};


template <class ...Observers>
using ref_subject = internal::subject<
    tupleref_base<Observers...> >;


}}
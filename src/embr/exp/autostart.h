#pragma once

#include <bitset>
#include <estd/internal/variadic.h>

// Service auto starter

// NOTE: Doing this with types directly, but IDs may be better to
// resolve cross dependencies

namespace embr { namespace experimental {

// Designed to be used on tuple::visit of all services
template <class Service>
struct service_starter_functor
{
    Service& service;
    int& counter;

    template <size_t I, class T, class Tuple, size_t S>
    bool operator()(estd::variadic::instance<I, T> vi, Tuple& tuple, std::bitset<S>& b)
    {
        using depends_on = typename T::depends_on;

        ++counter;

        if(b.test(I)) return false;

        b.set(I);

        // Recursively scan 'tuple' for depends_on
        tuple.visit(service_starter_functor<T>{vi.value, counter}, tuple, b);

        return false;
    }
};

}}

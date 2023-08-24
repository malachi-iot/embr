#pragma once

#include <estd/internal/variadic.h>

// Service auto starter

// NOTE: Doing this with types directly, but IDs may be better to
// resolve cross dependencies

namespace embr { namespace experimental {

// Designed to be used on tuple::visit of all services
struct service_starter_functor
{
    template <size_t I, class T, class Tuple>
    bool operator()(estd::variadic::instance<I, T> vi, Tuple& tuple) const
    {
        using depends_on = typename T::depends_on;

        // Recursively scan 'tuple' for depends_on

        return false;
    }
};

}}

#pragma once

#include <estd/type_traits.h>

#include <estd/internal/variadic.h>

// DEBT: Move this out of esp-idf specific area

namespace embr { inline namespace exp { namespace R {

// DEBT: Does a multi select.  My brain is melting right now, I know the logic is sound
// but I can't quite describe it
template <template <class> class E, class ...Types>
struct select_selector
{
    using requested_types = estd::variadic::types<Types...>;


    template <class T_j, size_t>
    using evaluator = typename requested_types::selector<E<T_j> >;
};


// Like is_same_selector, but instead evaluates all of ...Types to see
// if one of them is present
// DEBT: I'm thinking we can name this "any_selector" - but not sure
// DEBT: Once we work that out, put this into 'estd'
template <class ...Types>
struct is_in_selector
{
    using requested_types = estd::variadic::types<Types...>;


    template <class T_j>
    using types_contains = typename requested_types::where<estd::internal::is_same_selector<T_j> >;

    template <class T_j, size_t>
    using evaluator = estd::bool_constant<types_contains<T_j>::size() != 0>;
};


// DEBT: Cheating a bit, we don't want to ever specialize this way because
// we'd screw up scenarios where we want selector to move through many variadic::types
template <class ...Types>
struct is_in_selector<estd::variadic::types<Types...> > : is_in_selector<Types...>
{

};


// Selector which works on 'mux' variadic to select muxes based on particular rigid trait types
template <class ...Traits>
struct traits_selector
{
    using requested_traits = estd::variadic::types<Traits...>;

    // T_j is expected to be a 'mux'
    // DEBT: Improve this name
    // NOTE: This is backward, instead of searching in a mux for particular traits,
    // we want to search particular traits per mux.
    // i.e. we are searching that all traits in the mux are present in requested traits,
    // when we should be searching that all requested traits are present in the mux traits.
    // That said, it seems to compile anyway, belying expected behavior
    template <class T_j>
    using helper = typename T_j::traits::where<is_in_selector<Traits...> >;

    // Investigate whether all Traits are present for a given mux
    template <class Mux>
    using helper2 = typename requested_traits::where<is_in_selector<typename Mux::traits>>;

    // T_j is expected to be a 'mux'
    template <class T_j, size_t>
    using evaluator = estd::bool_constant<helper2<T_j>::size() == sizeof...(Traits)>;
};

}}}

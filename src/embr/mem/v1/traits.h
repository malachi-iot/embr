#pragma once

// DEBT: Make a span fwd
#include <estd/span.h>

#include "fwd.h"

namespace embr { namespace mem {

// DEBT: container_traits looking pretty useful.  Consider putting him up at estd level
// 08JAN26 MB - Backing off of this - std::data() and std::size() seem to do the job here (though
// the STD_VALUE_TYPE is still nice)
template <class T, int N>
struct container_traits<T[N]>
{
    static constexpr bool constexpr_size = true;

    //static constexpr int size() { return N; }

    using container_type = T[N];

    ESTD_CPP_STD_VALUE_TYPE(T)

    using iterator = pointer;

    //static T* data(container_type& c) { return c; }
};

template <class T, estd::size_t N>
struct container_traits<estd::span<T, N>>
{
    static constexpr bool constexpr_size = N != -1;

    //static constexpr int size() { return N; }

    using container_type = estd::span<T, N>;

    ESTD_CPP_STD_VALUE_TYPE(T)

    using iterator = pointer;

    //static pointer data(container_type& c) { return c.data(); }
};


/*
// DEBT: Guard with #if STD SPAN availability
template <class T, std::size_t N>
struct container_traits<std::span<T, N>>
{
    static constexpr bool constexpr_size = N != -1;

    //static constexpr int size() { return N; }

    using container_type = std::span<T, N>;

    ESTD_CPP_STD_VALUE_TYPE(T)

    using iterator = pointer;

    //static pointer data(container_type& c) { return c.data(); }
};  */


namespace detail { inline namespace v1 {

struct handles_traits_base
{
    using size_type = uint8_t;

    static constexpr size_type null = estd::numeric_limits<size_type>::max();
};

template <class Container>
struct handles_traits :
    handles_traits_base,
    container_traits<Container>
{
    using base_type = container_traits<Container>;
    using typename base_type::value_type;

    static constexpr bool is_null(const value_type& v) { return v.is_null(); }
    static void reset(value_type& v) { v.reset(); }

    struct bundle
    {
        value_type* value;
        size_type handle;
    };
};

}}


}}

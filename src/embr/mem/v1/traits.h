#pragma once

// DEBT: Make a span fwd
#include <estd/span.h>

#include "concepts.h"
#include "fwd.h"

namespace embr { namespace mem { inline namespace v1 {

#if !REFACTOR_CONTAINER_TRAITS
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
#endif

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

}

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
    using typename handles_traits_base::size_type;
    using container_type = Container;

    static constexpr bool is_null(const value_type& v) { return v.is_null(); }
    static void reset(value_type& v) { v.reset(); }
};

template <class Container>
struct pool_traits : estd::internal::container_traits<Container>
{
    static_assert(sizeof(typename estd::internal::container_traits<Container>::value_type) == 1);

    using block = v1::block;
};


// DEBT: Really we ought to directly use traits - waiting until we transition away from the need
// for Pool&
template <class Pool, ESTD_CPP_CONCEPT(concepts::Handles) Handles>
struct pool_ops_val_traits
{
    using pool_type = Pool;
    using handles_type = Handles;
    using storage_unref_type = estd::remove_cvref_t<Pool>;
    using handles_unref_type = estd::remove_cvref_t<Handles>;
    using storage_traits = typename storage_unref_type::pool_traits;
    using handles_traits = typename handles_unref_type::handles_traits;
};


// DEBT: Make concept for traits too
template <class StorageTraits, class HandlesTraits>
struct pool_ops_traits
{
    using storage_traits = StorageTraits;
    using handles_traits = HandlesTraits;

    using storage_type = pool<storage_traits>;
    using handles_type = handles<handles_traits>;

    using pool_type = storage_type;
};

template <class Pool, ESTD_CPP_CONCEPT(concepts::Handles) Handles>
using pool_ops_ref_traits = pool_ops_val_traits<Pool&, Handles&>;


}}


}}

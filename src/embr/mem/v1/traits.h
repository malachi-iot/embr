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

struct handles_traits_uint8
{
    using handle_type = uint8_t;

    static constexpr handle_type null = estd::numeric_limits<handle_type>::max();
};

// A little bit of a reversal, Page itself dictates some of the traits - this
// is to keep C++ error spew smaller
template <class Page>
struct page_traits
{
    using page_type = Page;
    using unit_type = typename page_type::unit_type;

    static constexpr unit_type zero = unit_type(0);

    static constexpr bool is_null(const page_type& v) { return v.is_null(); }
    static constexpr void reset(page_type& v) { v.reset(); }
    static constexpr bool is_zero(const page_type& v)
    {
        return v.pos() == zero;
    }
};

// <= c++14 needs this
template <class Page>
constexpr typename Page::unit_type page_traits<Page>::zero;

template <class Container>
struct handles_traits :
    handles_traits_uint8,
    container_traits<Container>,
    page_traits<typename container_traits<Container>::value_type>
{
    using base_type = container_traits<Container>;
    using typename base_type::value_type;
    using size_type = typename handles_traits_uint8::handle_type;
    using container_type = Container;
};

template <class Container>
struct pool_traits : estd::internal::container_traits<Container>
{
    static_assert(sizeof(typename estd::internal::container_traits<Container>::value_type) == 1);

    using block = v1::block_8;
};


// DEBT: Really we ought to directly use traits - waiting until we transition away from the need
// for Pool&
template <class Storage, ESTD_CPP_CONCEPT(concepts::Handles) Handles>
struct pool_ops_val_traits
{
    using storage_type = Storage;
    using handles_type = Handles;
    using storage_unref_type = estd::remove_cvref_t<Storage>;
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
};

}}


}}

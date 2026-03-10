#pragma once

#include <estd/internal/units/base.h>
#include <estd/internal/size.h>

#if __cpp_concepts
#include <concepts>
#endif

#include "pool/fwd.h"

// See https://github.com/malachi-iot/estdlib/issues/160
#define PAGE_ALIAS 0

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

// EXPERIMENTAL
// For types who inherently are aware of pooling
template <class T>
struct innate_traits
{
    static constexpr bool shared = false;
    static constexpr bool unique = false;
};

class small_block;

#if PAGE_ALIAS
// NOTE: Not possible because internal::units::unit_base has a protected default constructor
template <class Rep, unsigned alias = sizeof(void*)>
using page = estd::internal::units::unit_base<Rep, estd::ratio<alias>, page_tag>;
#else
template <class Rep, class Ratio = estd::ratio<sizeof(void*)>>
struct page;
#endif

template <bool B, class T>
using add_const_conditional_t = estd::conditional_t<B, estd::add_const_t<T>, T>;

template <class Derived>
class pool_crtp;

// Overlap with estd::experimental::global_provider

template <class T, T* t, bool global = t != nullptr>
struct global_provider;


template <class Pool, Pool* pool = nullptr>
class lock_handle;

template <class Pool, Pool* pool = nullptr>
class shared_handle;

template <class T, class Pool>
class typed_handle;

template <class Pool, Pool* pool>
class unique_handle;

}}

namespace internal { inline namespace v1 {

struct bundle;

//struct block;

}}


inline namespace v1 {

template <class T, class Pool, Pool* pool = nullptr>
class shared_handle;

template <class T, class Pool, Pool* pool = nullptr>
class lock_guard;

template <class T>
class pinned;

#if __cpp_deduction_guides
template <class T, class Pool, Pool* pool>
lock_guard(shared_handle<T, Pool, pool>) -> lock_guard<T, Pool, pool>;

template <class Pool, Pool* pool>
lock_guard(detail::v1::shared_handle<Pool, pool>) -> lock_guard<char, Pool, pool>;
#endif

template <class Container>
using container_traits = estd::internal::container_traits<Container>;

template <class T, class Pool, Pool* pool = nullptr>
class unique_handle;

}

template <class F, class Pool = void, Pool* pool = nullptr>
class function;

}}

#pragma once

#include <estd/internal/units/base.h>
#include <estd/internal/size.h>

#if __cpp_concepts
#include <concepts>
#endif

// See https://github.com/malachi-iot/estdlib/issues/160
#define PAGE_ALIAS 0

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

class block_8;

class small_block;

template <class Container>
struct handles_traits;

template <class Traits>
class handles;

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

template <class HandlesTraits, class Block = block_8>
struct bundle_base;


template <class Container>
struct pool_traits;

template <class Traits>
class pool;

template <class Derived>
class pool_crtp;

// Overlap with estd::experimental::global_provider

template <class T, T* t, bool global = t != nullptr>
struct global_provider;


template <class Pool, Pool* pool = {}>
class lock_handle;

template <class Pool, Pool* pool = {}>
class shared_handle;

}}

namespace internal { inline namespace v1 {

struct bundle;

//struct block;

}}


inline namespace v1 {

template <class T, class Pool, Pool* pool = {}>
class shared_handle;

template <class T, class Pool, Pool* pool>
class lock_guard;

#if __cpp_deduction_guides
template <class T, class Pool, Pool* pool>
lock_guard(shared_handle<T, Pool, pool>) -> lock_guard<T, Pool, pool>;

template <class Pool, Pool* pool>
lock_guard(detail::v1::shared_handle<Pool, pool>) -> lock_guard<char, Pool, pool>;
#endif

#define REFACTOR_CONTAINER_TRAITS 1

// DEBT: Refactor this into estd::internal::container_traits
#if REFACTOR_CONTAINER_TRAITS
template <class Container>
using container_traits = estd::internal::container_traits<Container>;
#else
template <class Container>
struct container_traits;
#endif

template <class T, class Pool, Pool* pool = nullptr>
class unique_handle;

}

template <class F, class Pool, Pool* pool = nullptr>
class function;

}}

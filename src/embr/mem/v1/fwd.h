#pragma once

#include <estd/internal/units/base.h>

#if __cpp_concepts
#include <concepts>
#endif

// See https://github.com/malachi-iot/estdlib/issues/160
#define PAGE_ALIAS 0

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class HandlesTraits, class Page>
struct bundle_base;

class block;

class small_block;

template <class Container>
struct handles_traits;

template <class Traits>
class handles;

struct page_tag {};

#if PAGE_ALIAS
// NOTE: Not possible because internal::units::unit_base has a protected default constructor
template <class Rep, unsigned alias = sizeof(void*)>
using page = estd::internal::units::unit_base<Rep, estd::ratio<alias>, page_tag>;
#else
template <class Rep, class Ratio = estd::ratio<sizeof(void*)>>
struct page;
#endif

template <class Container>
struct pool_traits;

template <class Traits>
class pool;

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

struct block;

}}


inline namespace v1 {

template <class T, class Pool, Pool* pool = {}>
class shared_handle;

template <class Container>
struct container_traits;


}


}}

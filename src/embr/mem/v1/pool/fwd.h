#pragma once

#include <estd/internal/macros.h>

#include "concepts.h"

#include "../enum.h"    // DEBT: Clumsy

#ifndef FEATURE_EMBR_GC_AGGRESSIVE_INLINE
#define FEATURE_EMBR_GC_AGGRESSIVE_INLINE 1
#endif

#if FEATURE_EMBR_GC_AGGRESSIVE_INLINE && !defined(__clang__)
#define EMBR_GC_FLATTEN __attribute__((flatten))
//#define EMBR_GC_INLINE inline __attribute__((always_inline))
//#define EMBR_GC_INLINE_FLATTEN inline __attribute__((always_inline, flatten))
#define EMBR_GC_TREE_SRA __attribute__((optimize("tree-sra")))
#define EMBR_GC_INLINE_BARE __attribute__((always_inline, optimize("tree-sra")))
#define EMBR_GC_INLINE inline EMBR_GC_INLINE_BARE
#define EMBR_GC_INLINE_FLATTEN inline __attribute__((always_inline, flatten, optimize("tree-sra")))
#else
#define EMBR_GC_FLATTEN
#define EMBR_GC_TREE_SRA
#define EMBR_GC_INLINE_BARE
#define EMBR_GC_INLINE
#define EMBR_GC_INLINE_FLATTEN
#endif

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

class block_8;

template <class HandlesTraits, class Block = block_8>
struct bundle_base;

template <class Container>
struct handles_traits;

template <ESTD_CPP_CONCEPT(concepts::HandlesTraits) Traits>
class handles;

template <class Container>
struct pool_traits;

template <class Traits>
class storage;

template <class Traits>
class pool_ops;

template <class T, class Traits, class ...Args>
typename pool_ops<Traits>::handle_type construct(pool_ops<Traits>& h, Args&&...args);

template <class Derived>
class pool_crtp;

template <class Derived, class Mutex = void>
class pool_mutex_crtp;

template <class T>
constexpr block_mode_enum::modes deduce_block_mode();

/// Override point for classes which may present as one block mode, but we want to
/// force to another
// DEBT: Overlap with innate_traits
template <class T>
struct item_traits;

template <class Pool, Pool* pool = nullptr>
class lock_handle;

template <class Pool, Pool* pool = nullptr>
class shared_handle;

template <class T, class Pool>
class sparse_handle;

template <class Pool, Pool* pool>
class unique_handle;

}}

namespace mixins { inline namespace v1 {

template <class Derived>
using pool = detail::pool_crtp<Derived>;

template <class Derived, class Mutex = void>
using pool_mutex = detail::pool_mutex_crtp<Derived, Mutex>;

}}

}}

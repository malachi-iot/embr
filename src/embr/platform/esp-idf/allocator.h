#pragma once

#include <limits>

#include <memory>

#include <estd/internal/platform.h>

#ifndef FEATURE_CAPS_ALLOCATOR_TRUE_MAX_SIZE
#define FEATURE_CAPS_ALLOCATOR_TRUE_MAX_SIZE 1
#endif

namespace embr { namespace esp_idf {

// TODO: Consider the void = T trick that greater/less pull - can we do it here?

template <class T, uint32_t caps, typename Size = uint32_t>
struct allocator
{
    ESTD_CPP_STD_VALUE_TYPE(T);

    using size_type = Size;

    static pointer allocate(size_type n, const void* hint = nullptr)
    {
        // TODO: Consider strongly using hint as an override for 'caps'
        return reinterpret_cast<pointer>(heap_caps_malloc(n * sizeof(T), caps));
    }

    static void deallocate(pointer p, size_type n)
    {
        heap_caps_free(p);
    }

    // DEBT: Once gcc actually removes this dependency, so should we
    template <class T2>
    struct rebind
    {
        using other = allocator<T2, caps>;
    };
};

}}

namespace std {

// Going down the line with
// https://en.cppreference.com/w/cpp/memory/allocator_traits

// Some conflicting guidance for whether we should specialize allocator_traits
// or come up with our own conforming allocator.  It seems the latter is really
// what is required:
// "A program that declares an explicit or partial specialization of std::allocator_traits is ill-formed, no diagnostic required."
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2652r1.html
// However, that begins with c++23 so we can squeak by for now.

template <class T, uint32_t caps>
struct allocator_traits<embr::esp_idf::allocator<T, caps> >   // NOLINT - see above c++23 commentary
{
    using allocator_type = embr::esp_idf::allocator<T, caps>;

    ESTD_CPP_STD_VALUE_TYPE(T);

    using difference_type = pointer_traits<pointer>::difference_type;
    using void_pointer = pointer_traits<pointer>::template rebind<void>;
    using const_void_pointer = pointer_traits<pointer>::template rebind<const void>;
    using size_type = make_unsigned_t<difference_type>;

    //template <class ...Args>
    //static void construct(allocator_type& a, pointer p, Args&&... args)
    template <class T2, class ...Args>
#if __cplusplus >= 201907L
    constexpr
#endif
    static void construct(allocator_type& a, T2* p, Args&&... args)
    {
        new (p) T2(std::forward<Args>(args)...);
    }

    template <class T2>
#if __cplusplus >= 201907L
    constexpr
#endif
    static void destroy(allocator_type& a, T2* p)
    {
        p->~T2();
    }

#if FEATURE_CAPS_ALLOCATOR_TRUE_MAX_SIZE
    constexpr static size_type max_size(allocator_type)
    {
        size_type sz = heap_caps_get_largest_free_block(caps);
        return sz / sizeof(value_type);
    }
#else
    constexpr static size_type max_size(allocator_type)
    {
        return std::numeric_limits<size_type>::max() / sizeof(value_type);
    }
#endif

    static pointer allocate(allocator_type& a, size_type n, const_void_pointer hint = nullptr)
    {
        return a.allocate(n, hint);
    }

    static void deallocate(allocator_type& a, pointer p, size_type n)
    {
        return a.deallocate(p, n);
    }

    // NOTE: c++ spec indicates this can supersede 'rebind' within alloc, but gcc 12.2
    // mandates 'rebind' be present
    template <class T2>
    using rebind_alloc = allocator_type::template rebind<T2>::other;

    template <class T2>
    using rebind_traits = allocator_traits<rebind_alloc<T2>>;

    using is_always_equal = is_empty<allocator_type>::type;

    // Used for scoped allocators, not really applicable to us just here for conformance
    static constexpr allocator_type select_on_container_copy_construction(
        const allocator_type&)
    {
        return {};
    }
};

}
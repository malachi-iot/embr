#pragma once

#include <estd/type_traits.h>
#include <estd/allocators/fixed.h>

#include "../netbuf.h"

namespace embr {

namespace experimental {

// conforms to estd's extended locking allocator signature
// considered a 'single' allocator, so applies to usage with estd::basic_string, etc.
// i.e. the buffer managed by this NetBufAllocator applies to only *one* allocated
// entity, no further 'alloc' calls allowed
template <class T, class TNetBuf>
class NetBufAllocator
{
    TNetBuf netbuf;

public:
    typedef const void* const_void_pointer;
    typedef typename estd::remove_reference<TNetBuf>::type netbuf_type;
    typedef bool handle_type; //single-allocator
    typedef handle_type handle_with_size;
    typedef T value_type;
    typedef T* pointer;
    typedef typename netbuf_type::size_type size_type;
    typedef estd::internal::handle_with_only_offset<handle_type, size_type> handle_with_offset;

private:
    // we're on a particular chunk, what absolute position does the beginning
    // of this chunk map to?
    size_type absolute_pos;

public:
    NetBufAllocator() : absolute_pos(0) {}

    /*
    NetBufAllocator(TNetBuf&& nb) :
        netbuf(std::move(nb)) {} */

    NetBufAllocator(const TNetBuf& nb) :
        netbuf(nb) {}

    /*
#ifdef FEATURE_CPP_MOVESEMANTIC
    template <class ... TArgs>
    NetBufAllocator(TArgs&&...args) :
        netbuf(std::forward<TArgs>(args)...),
        absolute_pos(0) {}
#endif */

    static CONSTEXPR handle_type invalid() { return false; }

    // technically we ARE locking since we have to convert the dummy 'bool' handle
    // to a pointer
    static CONSTEXPR bool is_locking() { return true; }

    static CONSTEXPR bool is_stateful() { return true; }

    static CONSTEXPR bool is_singular() { return true; }

    static CONSTEXPR bool is_contiguous() { return false; }

    static CONSTEXPR bool has_size() { return true; }

    // call not present in estd allocators. experimental
    size_type max_lock_size()
    {
        return netbuf.size();
    }

    size_type size(handle_with_size) const { return netbuf.total_size(); }

    value_type& lock(handle_type, size_type pos, size_type count)
    {
        // Netbuf will need a 'reset' to reposition chunk to the beginning
        // then, we'll need to count chunks forward until we arrive at pos.  Then,
        // we'll have to be extra sure 'count' is available.  Perhaps make going
        // past 'count' undefined, and demand caller heed max_lock_size.  Will be
        // tricky since 'pos' is from absolute start, not chunk start

        if(pos < absolute_pos)
        {
            netbuf.reset();
            absolute_pos = 0;

            while(netbuf.size() < pos)
            {
                pos -= netbuf.size();

                if(netbuf.last())
                {
                    embr::mem::ExpandResult r = netbuf.expand(pos, false);

                    if(r < 0)
                    {
                        // TODO: Report error, couldn't expand to requested position
                    }
                }

                absolute_pos += netbuf.size();
                netbuf.next();
            }
        }

        return *((char*)netbuf.data() + pos);

        // TODO: Perhaps we up any reference counter too, if NetBuf has one
    }

    void unlock(handle_type) {}
    void cunlock(handle_type) const {}

    const value_type& clock(handle_type h, size_type pos, size_type count) const
    {
        return const_cast<NetBufAllocator*>(this)->lock(h, pos, count);
    }

    handle_with_offset offset(handle_type h, size_t pos) const
    {
        return handle_with_offset(h, pos);
    }

    handle_with_size allocate_ext(size_type size)
    {
        // already allocated, shouldn't do it again
        return false;
    }

    handle_with_size reallocate_ext(handle_with_size, size_type size)
    {
        // already allocated, shouldn't do it again
        return false;
    }

    void deallocate(handle_type h, size_type count)
    {
    }

    handle_type reallocate(handle_type h, size_t len)
    {
        // Not yet supported operation, but netbufs (usually)
        // can do this
        assert(false);

        return h;
    }


    typedef typename estd::nothing_allocator<T>::lock_counter lock_counter;
};

}

}

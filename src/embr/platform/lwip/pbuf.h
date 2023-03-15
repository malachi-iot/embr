#pragma once

#include "udp.h"
#include "version.h"

#include "../guard-in.h"

// CONFIG_PBUF_CHAIN could be coming from esp-idf
#ifdef CONFIG_PBUF_CHAIN

// NOTE: Holding off spreading this around because we have a pending
// experimental merge that this would conflict with (exp/cleanup-pbuf)
#define ENABLE_EMBR_PBUF_CHAIN true
#endif


namespace embr { namespace lwip {

#ifdef FEATURE_CPP_INLINE_NAMESPACE
// The somewhat automatic behavior of these Pbuf wrappers can be
// confusing, and the naming as well.  Planning to revise this
// in v2
inline namespace v1 {
#endif

// If using this directly, know this is just a thin wrapper around
// pbuf itself.  No auto-reference magic
struct PbufBase
{
    typedef struct pbuf value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;

protected:
    pointer p;

public:
#ifdef FEATURE_CPP_DECLTYPE
    typedef decltype(p->len) size_type;
#else
    typedef uint16_t size_type;
#endif

    PbufBase(pointer p) : p(p) {}
    PbufBase() = delete;
    PbufBase(size_type size, pbuf_layer layer = PBUF_TRANSPORT) : 
        p(pbuf_alloc(layer, size, PBUF_RAM))
    {
    }

    // DEBT: Likely we need to consolidate these, given the bitwise
    // nature of constness
    const_pointer pbuf() const { return p; }
    pointer pbuf() { return p; }

    void* payload() const { return p->payload; }

    size_type length() const { return p->len; }

    size_type total_length() const 
    {
        return pbuf()->tot_len;
    }

    void realloc(size_type to_size)
    {
        pbuf_realloc(p, to_size);
    }

    void free()
    {
        pbuf_free(p);
    }

    size_type copy_partial(void* s, size_type len, size_type offset) const
    {
        return pbuf_copy_partial(p, s, len, offset);
    }

    void concat(pointer t)
    {
        pbuf_cat(p, t);
    }

    void chain(pointer t)
    {
        pbuf_chain(p, t);
    }

    /// "Skip a number of bytes at the start of a pbuf"
    /// @returns the pbuf in the queue where the offset is
    /// @link https://www.nongnu.org/lwip/2_1_x/group__pbuf.html#ga6a961522d81f0327aaf4d4ee6d96c583
    PbufBase skip(size_type in_offset, size_type* out_offset) const
    {
        return pbuf_skip(p, in_offset, out_offset);
    }

    err_t take(const void* dataptr, uint16_t len)
    {
        return pbuf_take(p, dataptr, len);
    }

    uint8_t get_at(uint16_t offset) const
    {
        return pbuf_get_at(p, offset);
    }

    void put_at(uint16_t offset, uint8_t data)
    {
        pbuf_put_at(p, offset, data);
    }

    bool valid() const { return p != NULLPTR; }

    PbufBase next() const { return p->next; }

#if LWIP_VERSION >= EMBR_LWIP_VERSION(2, 1, 0, 0)
#else
    pbuf_type type() const
    {
        return p->type;
    }
#endif

    operator pointer() const { return p; }
};

// returns size between the start of two pbufs
inline PbufBase::size_type delta_length(PbufBase from, PbufBase to)
{
    PbufBase::const_pointer p = from.pbuf();
    PbufBase::size_type len = 0;

    while(p != to.pbuf())
    {
        len += p->len;

        p = p->next;
    }

    return len;
}


/*
 * FIX: Something is wrong with tuple, so can't do this here
 * TODO: tuple may not work with C++98 anyway, though a simplistic implementation 
 * (enough for this use case) is possible
// returns pbuf and pbuf-relative offset at specified absolute position away from start
estd::tuple<PbufBase, PbufBase::size_type> delta_length(PbufBase start, PbufBase::size_type offset)
{
    PbufBase::pbuf_pointer p = start.pbuf();
    
    while(p != NULLPTR)
    {
        if(offset < p->len)
        {
            // return at this point, offset is within this pbuf
            return estd::tuple<PbufBase, PbufBase::size_type>(p, offset);
        }

        offset -= p->len;

        p = p->next;
    }

    return estd::tuple<PbufBase, PbufBase::size_type>(NULLPTR, 0);
}
*/

// This wraps pbuf + assists with allocation and reference
// management
struct Pbuf : PbufBase
{
    Pbuf(size_type size) : PbufBase(size)
    {
    }

    Pbuf(pointer p, bool bump_reference = true) : 
        PbufBase(p)
    {
        if(bump_reference) pbuf_ref(p);
    }

    Pbuf(const Pbuf& copy_from, bool bump_reference = true) :
        PbufBase(copy_from.p)
    {
        if(bump_reference) pbuf_ref(pbuf());
    }

#ifdef FEATURE_CPP_MOVESEMANTIC
    Pbuf(Pbuf&& move_from) :
        PbufBase(move_from.p)
    {
        move_from.p = NULLPTR;
    }
#endif

    ~Pbuf()
    {
        pointer p_to_free = pbuf();

        if(p_to_free != NULLPTR)
            // remember, pbufs are reference counted so this may or may not actually
            // deallocate pbuf memory
            pbuf_free(p_to_free);
    }
};

#ifdef FEATURE_CPP_INLINE_NAMESPACE
}   // v1
#else
namespace v1 {

typedef embr::lwip::PbufBase PbufBase;
typedef embr::lwip::Pbuf Pbuf;

}
#endif

}}


#include "../guard-out.h"

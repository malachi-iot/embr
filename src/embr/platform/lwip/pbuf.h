#pragma once

#include "udp.h"
#include "version.h"
#include "internal/pbuf.h"

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
struct PbufBase : lwip::internal::Pbuf
{
    typedef lwip::internal::Pbuf base_type;

    PbufBase(const base_type& copy_from) : base_type(copy_from) {}
    PbufBase(pointer p) : base_type(p) {}
    PbufBase() = delete;
    PbufBase(size_type size, pbuf_layer layer = PBUF_TRANSPORT) : 
        base_type(pbuf_alloc(layer, size, PBUF_RAM))
    {
    }


    // DEBT: Likely we need to consolidate these, given the bitwise
    // nature of constness
    const_pointer pbuf() const { return p; }
    pointer pbuf() { return p; }
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

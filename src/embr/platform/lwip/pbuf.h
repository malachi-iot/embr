/**
 * @file
 * 
 * Where netbuf-mk2 pbuf provider lives
 * 
 * Adapted from bronze-star PbufNetbufWrapper
 */

#pragma once

#include "udp.h"

#include <embr/netbuf.h>

// FIX: To be proper, we need to redefine these at the end if we can
#undef putchar
#undef puts
#undef putc

// CONFIG_PBUF_CHAIN could be coming from esp-idf
#ifdef CONFIG_PBUF_CHAIN
#define FEATURE_EMBR_PBUF_CHAIN_EXP
// NOTE: Holding off spreading this around because we have a pending
// experimental merge that this would conflict with (exp/cleanup-pbuf)
#define ENABLE_EMBR_PBUF_CHAIN true
#endif


namespace embr { namespace lwip {

// If using this directly, know this is just a thin wrapper around
// pbuf itself
struct PbufBase
{
    typedef struct pbuf pbuf_type;
    typedef pbuf_type* pbuf_pointer;
    typedef const pbuf_type* const_pbuf_pointer;

protected:
    pbuf_pointer p;

public:
    PbufBase(pbuf_pointer p) : p(p) {}
    PbufBase() {}

    const_pbuf_pointer pbuf() const { return p; }
    pbuf_pointer pbuf() { return p; }

#ifdef FEATURE_CPP_DECLTYPE
    typedef decltype(p->len) size_type;
#else
    typedef uint16_t size_type;
#endif

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

    size_type copy_partial(void* s, size_type len, size_type offset) const
    {
        return pbuf_copy_partial(p, s, len, offset);
    }

    // TODO: Do 'pbuf_at' for counterpoint to partial_length
};

// returns size between the start of two pbufs
PbufBase::size_type delta_length(PbufBase from, PbufBase to)
{
    PbufBase::const_pbuf_pointer p = from.pbuf();
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
    Pbuf(size_type size) : 
        PbufBase(pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM))
    {
    }

    Pbuf(pbuf_pointer p, bool bump_reference = true) : 
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
        pbuf_pointer p_to_free = pbuf();

        if(p_to_free != NULLPTR)
            // remember, pbufs are reference counted so this may or may not actually
            // deallocate pbuf memory
            pbuf_free(p_to_free);
    }
};


// netbuf-mk2 managing a lwip pbuf
// TODO: Extend from Pbuf, once I am in position to do regression
// testing
struct PbufNetbuf : PbufBase
{
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
    pbuf_pointer p_start;
#endif

public:
#ifdef FEATURE_CPP_DECLTYPE
    typedef decltype(p->len) size_type;
#else
    typedef uint16_t size_type;
#endif

    PbufNetbuf(size_type size) : 
        PbufBase(pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM))
    {
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
        p_start = p;
#endif
    }

    PbufNetbuf(pbuf_pointer p, bool bump_reference = true) : 
        PbufBase(p)
    {
        if(bump_reference) pbuf_ref(p);

#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
        p_start = p;
#endif
    }

    // FIX: Don't want to do reset here, but until seekoff gets sorted out,
    // we need this for testing
    PbufNetbuf(const PbufNetbuf& copy_from, bool reset, bool bump_reference = true) :
        PbufBase(copy_from.p)
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
        ,p_start(copy_from.p_start)
#endif
    {
        if(bump_reference) pbuf_ref(pbuf());

        if(reset) this->reset();
    }

#ifdef FEATURE_CPP_MOVESEMANTIC
    PbufNetbuf(PbufNetbuf&& move_from) :
        PbufBase(move_from.p)
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
        ,p_start(move_from.p_start)
#endif
    {
        move_from.p = NULLPTR;
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
        move_from.p_start = NULLPTR;
#endif
    }
#endif

    ~PbufNetbuf()
    {
        pbuf_pointer p_to_free = pbuf();

        if(p_to_free != NULLPTR)
            // remember, pbufs are reference counted so this may or may not actually
            // deallocate pbuf memory
            pbuf_free(p_to_free);
    }

#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
    static CONSTEXPR size_type threshold_size = 32;

    const_pbuf_pointer pbuf() const { return p_start; }
    pbuf_pointer pbuf() { return p_start; }
#else
    const_pbuf_pointer pbuf() const { return p; }
    pbuf_pointer pbuf() { return p; }
#endif

    operator const_pbuf_pointer() const { return pbuf(); }

    // p->len represents length of current pbuf, if a chain is involved
    // look at tot_len
    size_type size() const { return p->len; }

    size_type total_size() const 
    {
        return pbuf()->tot_len;
    }

    void* data() const { return p->payload; }

    bool last() const
    {
        return p->next == NULLPTR;
    }

    // Move pbuf forward to next in chain, if present
    bool next() 
    {
        if(last()) return false;

        p = p->next;

        return true;
    }

    // lightly tested
    embr::mem::ExpandResult expand(size_type by_size, bool move_to_next)
    {
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
        // threshold tells us minimum size to allocate by
        if(by_size < threshold_size)
            by_size = threshold_size;
            
        pbuf_pointer new_p = pbuf_alloc(PBUF_TRANSPORT, by_size, PBUF_RAM);

        if(new_p == NULLPTR) return embr::mem::ExpandFailOutOfMemory;

        // assumes we called expand while at the end of p chain

        // lightly tested
        pbuf_cat(p_start, new_p);

        // TODO: Might just assign p to new_p
        if(move_to_next) p = p->next;
        
        return embr::mem::ExpandOKChained;
#else
        return embr::mem::ExpandFailFixedSize;
#endif
    }

    // EXPERIMENTAL and lightly tested
    void shrink(size_type to_size)
    {
        pbuf_realloc(pbuf(), to_size);
    }

    // moves pbuf chain back to beginning
    void reset()
    {
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
        p = p_start;
#endif
    }


    // DEBUG ONLY
    // counts number of active chains
    // does not (always) check for null pbuf
    int chain_counter() const
    {
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
        pbuf_pointer p_counter = p_start;
        int counter = 0;
        while(p_counter)
        {
            counter++;
            p_counter = p_counter->next;
        }
        return counter;
#else
        return 1;
#endif
    }
};

}}
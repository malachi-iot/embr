#pragma once

#include <embr/obsolete/netbuf.h>


namespace embr { namespace lwip { namespace legacy {


// netbuf-mk2 managing a lwip pbuf
// TODO: Extend from Pbuf, once I am in position to do regression
// testing
// NOTE: Obsolete, though potentially still useable.  Is underpinning of
// legacy netbuf-streambuf code which is obsolete and should not be used
struct PbufNetbuf : PbufBase
{
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
    pointer p_start;
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

    PbufNetbuf(pointer p, bool bump_reference = true) : 
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
        pointer p_to_free = pbuf();

        if(p_to_free != NULLPTR)
            // remember, pbufs are reference counted so this may or may not actually
            // deallocate pbuf memory
            pbuf_free(p_to_free);
    }

#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
    static CONSTEXPR size_type threshold_size = 32;

    const_pointer pbuf() const { return p_start; }
    pointer pbuf() { return p_start; }
#else
    const_pointer pbuf() const { return p; }
    pointer pbuf() { return p; }
#endif

    operator const_pointer() const { return pbuf(); }

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
            
        pointer new_p = pbuf_alloc(PBUF_TRANSPORT, by_size, PBUF_RAM);

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
        pointer p_counter = p_start;
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


}}}
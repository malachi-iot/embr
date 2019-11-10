/**
 * @file
 * 
 * Where netbuf-mk2 pbuf provider lives
 * 
 * Adapted from bronze-star PbufNetbufWrapper
 */

#pragma once

extern "C" {

#include <lwip/api.h>
#include <lwip/udp.h>

}

#include <estd/internal/platform.h>
#include <embr/netbuf.h>

#undef putchar
#undef puts
#undef putc

// CONFIG_PBUF_CHAIN could be coming from esp-idf
#ifdef CONFIG_PBUF_CHAIN
#define FEATURE_EMBR_PBUF_CHAIN_EXP
#endif


namespace embr { namespace lwip {

struct PbufNetbufBase
{
    typedef struct pbuf pbuf_type;
    typedef pbuf_type* pbuf_pointer;

private:
};

// netbuf-mk2 managing a lwip pbuf
struct PbufNetbuf : PbufNetbufBase
{

private:
    pbuf_pointer p;
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
    pbuf_pointer p_start;
#endif

public:
#ifdef FEATURE_CPP_DECLTYPE
    typedef decltype(p->len) size_type;
#else
    typedef uint16_t size_type;
#endif

    PbufNetbuf(size_type size)
    {
        p = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM);

#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
        p_start = p;
#endif
    }

    PbufNetbuf(pbuf_pointer p, bool bump_reference = true) : p(p)
    {
        if(bump_reference) pbuf_ref(p);

#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
        p_start = p;
#endif
    }

    PbufNetbuf(const PbufNetbuf& copy_from, bool bump_reference = true) :
        p(copy_from.p)
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
        ,p_start(copy_from.p_start)
#endif
    {
        pbuf_pointer p_to_bump =
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
            p_start;
#else
            p;
#endif

        if(bump_reference) pbuf_ref(p_to_bump);
    }

#ifdef FEATURE_CPP_MOVESEMANTIC
    PbufNetbuf(PbufNetbuf&& move_from) :
        p(move_from.p)
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
        pbuf_pointer p_to_free =
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
            p_start;
#else
            p;
#endif

        if(p_to_free != NULLPTR)
            // remember, pbufs are reference counted so this may or may not actually
            // deallocate pbuf memory
            pbuf_free(p_to_free);
    }

#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
    static CONSTEXPR size_type threshold_size = 32;
#endif

    // p->len represents length of current pbuf, if a chain is involved
    // look at tot_len
    size_type size() const { return p->len; }

    size_type total_size() const 
    {
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
        return p_start->tot_len;
#else
        return p->tot_len;
#endif
    }

    uint8_t* data() const { return (uint8_t*) p->payload; }

    bool next() 
    {
        // TODO: Look into whether there's a particular pbuf API
        // call that's appropriate (probably not)
        if(p->next != NULLPTR)
        {
            p = p->next;
            return true;
        }

        return false;
    }

    // EXPERIMETNAL and lightly tested
    embr::mem::ExpandResult expand(size_type by_size, bool move_to_next)
    {
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
        if(by_size < threshold_size)
            by_size = threshold_size;
            
        pbuf_pointer new_p = pbuf_alloc(PBUF_TRANSPORT, by_size, PBUF_RAM);

        if(new_p == NULLPTR) return embr::mem::ExpandFailOutOfMemory;

        // assumes we called expand while at the end of p chain

        // UNTESTED
        pbuf_cat(p_start, new_p);

        // TODO: Might just assign p to new_p
        if(move_to_next) p = p->next;
        
        return embr::mem::ExpandOKChained;
#else
        return embr::mem::ExpandFailFixedSize;
#endif
    }

    // EXPERIMENTAL
    void shrink(size_type to_size)
    {
        pbuf_realloc(
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
            p_start, 
#else
            p,
#endif
            to_size);
    }
};

}}
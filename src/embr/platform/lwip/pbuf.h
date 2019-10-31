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

namespace embr { namespace lwip {

// netbuf-mk2 managing a lwip pbuf
struct PbufNetbuf
{
    typedef struct pbuf pbuf_type;
    typedef pbuf_type* pbuf_pointer;
    typedef unsigned short size_type;

private:
    pbuf_pointer p; 

public:
    PbufNetbuf(size_type size)
    {
        p = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM);
    }

#ifdef FEATURE_CPP_MOVESEMANTIC
    PbufNetbuf(PbufNetbuf&& move_from) :
        p(move_from.p)
    {
        move_from.p = NULLPTR;
    }
#endif

    ~PbufNetbuf()
    {
        if(p != NULLPTR)
            // remember, pbufs are reference counted so this may or may not actually
            // deallocate pbuf memory
            pbuf_free(p);
    }

    // p->len represents length of current pbuf, if a chain is involved
    // look at tot_len
    size_type size() const { return p->len; }

    uint8_t* data() const { return (uint8_t*) p->payload; }

    bool next() { return false; }

    embr::mem::ExpandResult expand(size_type by_size, bool move_to_next)
    { 
        return embr::mem::ExpandFailFixedSize;
    }
};

}}
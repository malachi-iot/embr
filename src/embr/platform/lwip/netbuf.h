/**
 * 
 * References:
 * 
 * 1. https://www.nongnu.org/lwip/2_0_x/group__netbuf.html
 */
#pragma once

extern "C" {

#include <lwip/api.h>
#include <lwip/netbuf.h>

}

#include <estd/internal/platform.h>

namespace embr { namespace lwip {

class Netbuf
{
    typedef struct netbuf value_type;
    typedef value_type* pointer;

    pointer buf;

public:
    void _new()
    {
        buf = netbuf_new();
    }

    void* alloc(uint16_t size)
    {
        return netbuf_alloc(buf, size);
    }

    /**
     * @brief "Chain one netbuf to another" [1]
     * 
     * @param tail 
     */
    void chain(pointer tail)
    {
        return netbuf_chain(buf, tail);
    }

    void del()
    {
        netbuf_delete(buf);
    }

    void first()
    {
        netbuf_first(buf);
    }


    void free()
    {
        netbuf_free(buf);
    }


    int8_t next()
    {
        return netbuf_next(buf);
    }

    err_t ref(const void* dataptr, uint16_t size)
    {
        return netbuf_ref(buf, dataptr, size);
    }
};

}}
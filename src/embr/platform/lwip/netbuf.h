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
    bool alloc(uint16_t size)
    {
        buf = netbuf_alloc(size);
        return buf != NULLPTR;
    }

    void delete()
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
}

}}
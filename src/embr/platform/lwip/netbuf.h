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
#include <estd/span.h>

#include "endpoint.h"

namespace embr { namespace lwip {

class Netconn;

class Netbuf
{
protected:
    typedef struct netbuf value_type;
    typedef value_type* pointer;

    friend class Netconn;

    pointer buf;

public:
    pointer b() const { return buf; }

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

    err_t data(void** dataptr, uint16_t* len)
    {
        return netbuf_data(buf, dataptr, len);
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

    const ip_addr_t* fromaddr() const
    {
        return netbuf_fromaddr(buf);
    }

    uint16_t fromport() const
    {
        return netbuf_fromport(buf);
    }

    Endpoint fromendpoint() const
    {
        return Endpoint(fromaddr(), fromport());
    }

    operator pointer() { return buf; }
};


namespace experimental {

class AutoNetbuf : public Netbuf
{
    friend class AutoNetconn;

    err_t err;
    
    AutoNetbuf(pointer netbuf)
    {
        buf = netbuf;
    }

public:
    AutoNetbuf()
    {
        _new();
    }


    ~AutoNetbuf()
    {
        del();
    }

    err_t data(void** dataptr, uint16_t* len)
    {
        return err = Netbuf::data(dataptr, len);
    }

    estd::span<estd::byte> data()
    {
        estd::byte* d;
        uint16_t len;

        data((void**)&d, &len);

        return estd::span<estd::byte>(d, len);
    }  

    err_t last_err() const { return err; }
};

}

}}
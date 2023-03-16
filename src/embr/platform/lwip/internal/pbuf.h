#pragma once

#include "../udp.h"

namespace embr { namespace lwip { namespace internal {

struct Pbuf
{
    typedef struct pbuf value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;

protected:
    pointer p;

    ESTD_CPP_DEFAULT_CTOR(Pbuf)
    Pbuf(pointer p) : p(p) {}
    Pbuf(const Pbuf& copy_from) : p(copy_from.p) {}

public:
#ifdef FEATURE_CPP_DECLTYPE
    typedef decltype(p->len) size_type;
#else
    typedef uint16_t size_type;
#endif

    void* payload() const { return p->payload; }

    size_type length() const { return p->len; }

    size_type total_length() const 
    {
        return p->tot_len;
    }

    void realloc(size_type to_size)
    {
        pbuf_realloc(p, to_size);
    }

    void free()
    {
        pbuf_free(p);
    }

    void ref()
    {
        pbuf_ref(p);
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
    Pbuf skip(size_type in_offset, size_type* out_offset) const
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

    Pbuf next() const { return p->next; }

#if LWIP_VERSION >= EMBR_LWIP_VERSION(2, 1, 0, 0)
#else
    pbuf_type type() const
    {
        return p->type;
    }
#endif

    operator pointer() const { return p; }

    bool ESTD_CPP_CONSTEXPR_RET is_null() const { return p == nullptr; }
};

}}}
#pragma once

#include "../internal/pbuf.h"
#include "../version.h"
#include "../../../internal/scoped_guard.h"

namespace embr { namespace lwip {

#ifdef FEATURE_CPP_INLINE_NAMESPACE
inline
#endif

namespace v2 {

struct Pbuf : embr::lwip::internal::Pbuf
{
    typedef embr::lwip::internal::Pbuf base_type;

    ESTD_CPP_FORWARDING_CTOR(Pbuf)

    pointer alloc(pbuf_layer layer, uint16_t length, pbuf_type type = PBUF_RAM)
    {
        return p = pbuf_alloc(layer, length, type);
    }

    pointer alloc(pbuf_layer layer, uint16_t length, pbuf_type type, struct pbuf_custom* pc,
        void* payload_mem, uint16_t payload_mem_len)
    {
        return p = pbuf_alloced_custom(layer, length, type, pc, payload_mem, payload_mem_len);
    }
};

}

}

namespace internal {

template <>
struct scoped_guard_traits<lwip::v2::Pbuf>
{
    typedef int status_type;
};



template <>
struct scoped_guard<lwip::v2::Pbuf> :
    scoped_guard_base<lwip::v2::Pbuf, false, SCOPED_GUARD_WARN>
{
    typedef scoped_guard_base<lwip::v2::Pbuf, false, SCOPED_GUARD_WARN> base_type;

    scoped_guard(const scoped_guard& copy_from)
    {
        base_type::value_ = copy_from.value_;
        base_type::value().ref();
    }

    scoped_guard(scoped_guard&& move_from)
    {
        base_type::value_ = move_from.value_;
        move_from.value_ = nullptr;
    }

    scoped_guard(lwip::internal::Pbuf::size_type size, pbuf_layer layer = PBUF_TRANSPORT)
    {
        value().alloc(layer, size);
    }

    ~scoped_guard()
    {
        if(value().is_null()) return;

        value().free();
    }

    ESTD_CPP_CONSTEXPR_RET bool good() const { return !value().is_null(); }
};


}

}
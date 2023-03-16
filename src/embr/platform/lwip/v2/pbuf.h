#pragma once

#include "../internal/pbuf.h"
#include "../version.h"
#include "../../../internal/scoped_guard.h"

namespace embr { namespace lwip {

// Only activate this inline once we've fully migrated from v1 pbuf
#if defined(FEATURE_CPP_INLINE_NAMESPACE) && FEATURE_EMBR_LWIP_PBUF_VERSION == 2
inline
#endif

namespace v2 {

struct Pbuf : embr::lwip::internal::Pbuf
{
    typedef embr::lwip::internal::Pbuf base_type;

    ESTD_CPP_FORWARDING_CTOR(Pbuf)

    pointer alloc(uint16_t length, pbuf_type type = PBUF_RAM)
    {
        return p = pbuf_alloc(PBUF_TRANSPORT, length, type);
    }

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
    // As per https://www.nongnu.org/lwip/2_0_x/group__infrastructure__errors.html
    typedef err_enum_t status_type;
};



template <scoped_guard_fail_action fail_action>
struct scoped_guard<lwip::v2::Pbuf, fail_action> :
    scoped_guard_base<lwip::v2::Pbuf, false, fail_action>
{
    typedef scoped_guard_base<lwip::v2::Pbuf, false, fail_action> base_type;

    ESTD_CPP_CONSTEXPR_RET bool empty() const { return base_type::value().is_null(); }
    ESTD_CPP_CONSTEXPR_RET bool good() const { return empty() == false; }

    ESTD_CPP_CONSTEXPR_RET scoped_guard(const scoped_guard& copy_from)
    {
        base_type::value_ = copy_from.value_;
        base_type::value().ref();
    }

#if __cpp_rvalue_references
    constexpr scoped_guard(scoped_guard&& move_from)
    {
        base_type::value_ = move_from.value_;
        move_from.value_ = nullptr;
    }
#endif

    scoped_guard(lwip::internal::Pbuf::size_type size, pbuf_layer layer = PBUF_TRANSPORT)
    {
        base_type::value().alloc(layer, size);
        if(empty()) base_type::status(ERR_MEM);
    }

    ~scoped_guard()
    {
        if(empty()) return;

        base_type::value().free();
    }
};


}

}
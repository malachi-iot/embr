#pragma once

#include "../v1/pbuf.h"
#include "../version.h"
#include "../../../internal/scoped_guard.h"

namespace embr { namespace lwip {

#ifdef FEATURE_CPP_INLINE_NAMESPACE
inline
#endif

namespace v2 {

typedef embr::lwip::v1::PbufBase Pbuf;

}

}

namespace internal {

template <>
struct scoped_guard_traits<lwip::v2::Pbuf>
{
    typedef int status_type;
};


/*
template <>
struct scoped_guard<lwip::v2::Pbuf> :
    scoped_guard_base<lwip::v2::Pbuf, false, SCOPED_GUARD_WARN>
{
    typedef scoped_guard_base<lwip::v2::Pbuf, false, SCOPED_GUARD_WARN> base_type;

    scoped_guard() : base_type(nullptr) {}
};
*/

}

}
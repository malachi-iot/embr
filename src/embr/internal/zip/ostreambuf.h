#pragma once

#include <estd/internal/impl/streambuf.h>

#include "headers.h"

namespace embr { namespace zip { namespace impl {

// Limitations:
// 1. Doesn't append to existing zip stream
// 2. 100% uncompressed

template <class Streambuf>
class container_ostreambuf :
    public estd::internal::impl::wrapped_streambuf_base<Streambuf>
{
    using base_type = estd::internal::impl::wrapped_streambuf_base<Streambuf>;

public:
    ESTD_CPP_FORWARDING_CTOR(container_ostreambuf)

    // Demarcate a new file entry
    template <class TimePoint>
    void file(char* name, TimePoint stamp, uint32_t len, uint32_t crc);
};

}}}
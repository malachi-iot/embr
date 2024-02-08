#pragma once

#include <estd/variant.h>
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

    estd::variant<
        header::layer1::local_file<128> > state_;

public:
    ESTD_CPP_FORWARDING_CTOR(container_ostreambuf)

    // Demarcate a new file entry
    template <class Impl, class TimePoint>
    void file(const estd::detail::basic_string<Impl>& name, TimePoint stamp)
    {
        // NOTE: Our variant has additional get here to bypass runtime checks.
        // Therefore, it's incumbent on US to make sure we're in the right state
        header::layer1::local_file<128>* lf = state_.get<0>();
        lf->h.flags = header::flags::data_descriptor;
        lf->filename(estd::layer2::const_string(name));

        //base_type::rdbuf().sputn(lf, 0);
    }
};

}}}
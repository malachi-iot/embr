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

    unsigned offset_ = 0;
    uint32_t crc = 0;
    estd::layer1::string<128> filename_;

    estd::variant<
        header::local_file,
        header::data_descriptor,
        header::central_directory> state_;

    unsigned chunk_size(unsigned compressed_size)
    {
        return compressed_size +
            sizeof(header::local_file) +
            sizeof(header::data_descriptor) +
            sizeof(header::central_directory) +
            filename_.size() * 2;
    }

public:
    using typename base_type::char_type;
    using typename base_type::int_type;

    ESTD_CPP_FORWARDING_CTOR(container_ostreambuf)

    // Demarcate a new file entry
    template <class Impl, class TimePoint>
    void file(const estd::detail::basic_string<Impl>& name, TimePoint stamp)
    {
        // NOTE: Our variant has additional get here to bypass runtime checks.
        // Therefore, it's incumbent on US to make sure we're in the right state
        //header::layer1::local_file<128>* lf = state_.get<0>();
        // DEBT: Darnit, above got data hidden...
        //header::layer1::local_file<128>* lf = &get<0>(state_);
        header::local_file* lf = estd::internal::get_ll<0>(state_);

        // TODO: Zero out all the stuff in lf

        lf->h.flags = header::flags::data_descriptor;
        lf->length.filename = name.length();
        filename_ = name;
        //name.copy(filename_, lf->length.filename);

        base_type::rdbuf().sputn((char*)lf, sizeof(header::local_file));
        base_type::rdbuf().sputn(filename_.data(), lf->length.filename);

        state_ = header::data_descriptor{};
    }

    template <class TimePoint>
    void file(const char* name, TimePoint stamp)
    {
        file(estd::layer2::const_string(name), stamp);
    }

    void finalize_file()
    {
        //header::data_descriptor* dd = state_.get<1>();
        header::data_descriptor* dd = estd::internal::get_ll<1>(state_);

        dd->uncompressed_size = dd->compressed_size;

        base_type::rdbuf().sputn((char*)dd, sizeof(header::data_descriptor));

        state_ = header::central_directory {};

        header::central_directory* cd = estd::internal::get_ll<2>(state_);

        cd->local_file_offset = offset_;
        cd->length.filename = filename_.length();

        base_type::rdbuf().sputn((char*)cd, sizeof(header::central_directory));
        base_type::rdbuf().sputn(filename_.data(), filename_.length());
    }


    int xsputn(const char_type* data, int sz)
    {
        header::data_descriptor* dd = estd::internal::get_ll<1>(state_);

        dd->compressed_size += sz;
        // TODO: Update checksum

        return base_type::rdbuf().xsputn(data, sz);
        //return sz;
    }
};

}

template <class Streambuf>
using container_ostreambuf =
    estd::detail::streambuf<impl::container_ostreambuf<Streambuf> >;

template <class Streambuf>
using container_ostream =
    estd::detail::basic_ostream<container_ostreambuf<Streambuf> >;


}

// DEBT: Put this elsewhere

}
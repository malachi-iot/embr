#pragma once

#include <estd/istream.h>

#include "decoder.h"

namespace embr { namespace json {

namespace internal {

template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
void decoder::decode_rdbuf(const Streambuf& sb, F&& f)
{

}

template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class Base, class F>
void decoder::decode(estd::detail::basic_istream<Streambuf, Base>& in, F&& f)
{
    //using in_type = estd::detail::basic_istream<InImpl, Base>;

    // FIX: Feels very wrong that in.rdbuf() is const
    decode_rdbuf(in.rdbuf(), std::forward<F>(f));
}

}

}}

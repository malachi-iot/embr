#pragma once

#include <estd/iosfwd.h>

namespace embr { namespace json {

inline namespace v1 {

}

namespace internal {

class decoder
{
    template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
    void decode_rdbuf(const Streambuf& sb, F&& f);

public:
    // DEBT: Paradigm collision with something like minij::modes
    enum states
    {
        IDLE,

    };

    // https://datatracker.ietf.org/doc/html/rfc8259#section-3
    enum items
    {
        OBJECT,
        ARRAY,
        NUMBER,
        STRING,
        BOOL
    };

    template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class Base, class F>
    void decode(estd::detail::basic_istream<Streambuf, Base>& in, F&& f);
};

}

}}

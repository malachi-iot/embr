#pragma once

#include <estd/iosfwd.h>

namespace embr { namespace json {

inline namespace v1 {

}

namespace internal {

class decoder_state
{
public:
    // DEBT: Paradigm collision with something like minij::modes
    enum states
    {
        IDLE,
        TOKEN_START,
        TOKEN_END,
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

protected:
    states state_{};
    const decoder_state* const parent_;
    items item_;

public:
    decoder_state(const decoder_state* parent = nullptr) : parent_{parent}    {}
};

class decoder : public decoder_state
{
    template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
    void decode_rdbuf(Streambuf& sb, F&& f);

    template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
    void decode_idle(Streambuf& sb, F&& f);

    template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
    void decode_token(Streambuf& sb, F&& f);

    template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
    void decode_string(Streambuf& sb, F&& f);

    template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
    struct worker : decoder_state
    {

    };

public:

    template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class Base, class F>
    void decode(estd::detail::basic_istream<Streambuf, Base>& in, F&& f);

private:
    states state_{};
    const decoder* const parent_;
    items item_;

public:
    decoder(const decoder* parent = nullptr) : parent_{parent}    {}
};

}

}}

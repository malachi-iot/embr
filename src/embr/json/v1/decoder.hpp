#pragma once

#include <estd/istream.h>

#include "decoder.h"

namespace embr { namespace json {

namespace internal {

template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
void decoder::decode_string(Streambuf& sb, F&& f)
{
    using traits = typename Streambuf::traits_type;
    using char_type = typename traits::char_type;
    using int_type = typename traits::int_type;
    using pointer = const char_type*;
    // Hmm, istringbuf why don't you have this?
    //pointer data = sb.gptr();

    // DEBT: Can't just toss a big temporary like this around
    char temp[64];
    int idx = 0;
    int_type c;

    while((c = sb.sbumpc()) != '"')
    {
        temp[idx++] = c;
    }

    temp[idx] = 0;
    f(*this);
    state_ = TOKEN_END;
}

template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
void decoder::decode_token(Streambuf& sb, F&& f)
{
    using traits = typename Streambuf::traits_type;
    using char_type = typename traits::char_type;

    const char_type c = sb.sgetc();

    switch(item_)
    {
        case STRING:
            decode_string(sb, std::forward<F>(f));
            break;

        case OBJECT:
            if(c == '}') state_ = TOKEN_END;
            break;

        case ARRAY:
            if(c == ']') state_ = TOKEN_END;
            break;

        default:
            if(c == ',') state_ = TOKEN_END;
            break;
    }
}

template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
void decoder::decode_idle(Streambuf& sb, F&& f)
{
    using traits = typename Streambuf::traits_type;
    using int_type = typename traits::int_type;

    const int_type c = sb.sbumpc();

    switch(c)
    {
        case '[':
        {
            item_ = ARRAY;
            state_ = TOKEN_START;
            decoder child(this);
            child.decode_rdbuf(sb, std::forward<F>(f));
            break;
        }

        case ']':
        {
            break;
        }

        case '{':
        {
            item_ = OBJECT;
            state_ = TOKEN_START;
            decoder child(this);
            child.decode_rdbuf(sb, std::forward<F>(f));
            break;
        }

        case '}':
            state_ = TOKEN_END;
            break;

        case '"':
            item_ = STRING;
            state_ = TOKEN_START;
            //f(*this);
            break;

        case ' ': break;

        default:
            break;

        case traits::eof():
            break;
    }
}

template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
void decoder::decode_rdbuf(Streambuf& sb, F&& f)
{
    for(;;)
    {
        switch(state_)
        {
            case IDLE:
                decode_idle(sb, std::forward<F>(f));
                break;

            case TOKEN_START:
                decode_token(sb, std::forward<F>(f));
                break;

            case TOKEN_END:
                return;

            default:    break;
        }
    }
}

template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class Base, class F>
void decoder::decode(estd::detail::basic_istream<Streambuf, Base>& in, F&& f)
{
    //using in_type = estd::detail::basic_istream<InImpl, Base>;

    decode_rdbuf(*in.rdbuf(), std::forward<F>(f));
}

}

}}

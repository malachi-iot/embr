#pragma once

#include <estd/istream.h>

#include "decoder.h"
#include "../../internal/breadcrumb.h"

namespace embr { namespace json {

namespace internal {

enum literal_ids
{
    ID_TRUE,
    ID_FALSE,
    ID_NULL
};

// Breadcrumbs want to be sorted per name per parent.  These all shame the same
// top level parent.  ID is not sorted here.
constexpr embr::internal::breadcrumb literals[]
{
    { "false",  ID_FALSE },
    { "null",   ID_NULL },
    { "true",   ID_TRUE },
};

template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
void decoder::worker<Streambuf, F>::decode_string(Streambuf& sb, F&& f)
{
    using pointer = const char_type*;

    int idx = 0;
    int_type c;

    while((c = sb.sbumpc()) != '"')
    {
        ++idx;
    }

    int pos = sb.pubseekoff(-(idx + 1), estd::ios_base::cur, estd::ios_base::in);     // skip closing '"'
    f(*this, item { idx });
    sb.pubseekpos(pos + idx + 1, estd::ios_base::in);
    state_ = TOKEN_END;
}


template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
void decoder::worker<Streambuf, F>::decode_literal(Streambuf& sb, F&& f)
{
    embr::internal::searcher searcher{json::internal::literals};

    while(searcher.search(sb.sbumpc(), [](auto){ return true; }) == searcher.SEARCHING)   {}

    const int idx = searcher.pos;
}


template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
void decoder::worker<Streambuf, F>::decode_number(Streambuf& sb, F&& f)
{
    int_type c;
    int idx = 0;

    while((c = sb.sbumpc()) == '.' || estd::isdigit(c)) ++idx;

    int pos = sb.pubseekoff(-(idx + 1), estd::ios_base::cur, estd::ios_base::in);
    f(*this, item { idx });
    sb.pubseekpos(pos + idx + 1, estd::ios_base::in);

    state_ = TOKEN_END;
}


template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
void decoder::worker<Streambuf, F>::decode_token(Streambuf& sb, F&& f)
{
    const char_type c = sb.sgetc();

    switch(item_)
    {
        case STRING:
            decode_string(sb, std::forward<F>(f));
            break;

        case NUMBER:
            decode_number(sb, std::forward<F>(f));
            break;

        case LITERAL:
            decode_literal(sb, std::forward<F>(f));
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
void decoder::worker<Streambuf, F>::decode_idle(Streambuf& sb, F&& f)
{
    const int_type c = sb.sbumpc();

    switch(c)
    {
        case '[':
        {
            item_ = ARRAY;
            state_ = TOKEN_START;
            worker child(this);
            child.decode_rdbuf(sb, std::forward<F>(f));
            break;
        }

        case '{':
        {
            item_ = OBJECT;
            state_ = TOKEN_START;
            worker child(this);
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
void decoder::worker<Streambuf, F>::decode_rdbuf(Streambuf& sb, F&& f)
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
    worker<Streambuf, F> w;

    w.decode_rdbuf(*in.rdbuf(), std::forward<F>(f));
}

}

}}

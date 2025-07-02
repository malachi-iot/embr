#pragma once

#include <estd/istream.h>

#include "decoder.h"
#include "../../internal/breadcrumb.h"

namespace embr { namespace json {

namespace internal {

// Breadcrumbs want to be sorted per parent ID per name.  These all shame the same
// top level parent.  child ID is not sorted.
constexpr embr::internal::breadcrumb literals[]
{
    { "false",  ID_FALSE },
    { "null",   ID_NULL },
    { "true",   ID_TRUE },
    { },
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
    return;

    // Nearly works, but since incoming sb doesn't provide \0 termination, searcher gets confused
    // Also searcher seems to get TOO confused and segfaults when searching at thes end
    embr::internal::searcher::results r;
    embr::internal::searcher searcher{json::internal::literals};

    while((r = searcher.search(sb.sbumpc(), [](auto){ return true; })) == searcher.SEARCHING)
    {

    }

    if(r == searcher.MATCHED)
    {
        const int idx = searcher.pos;
        auto id = static_cast<literal_ids>(searcher.marker_->id);

        f(*this, item { id });
    }
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
    estd::remove_const_t<char_type> c;

    switch(item_)
    {
        case NAME:
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
            c = sb.sbumpc();
            if(c == ':')
            {
                worker child(this);
                child.decode(sb, std::forward<F>(f));
            }
            if(c == '}') state_ = TOKEN_END;
            break;

        case ARRAY:
            c = sb.sbumpc();
            if(c == ']') state_ = TOKEN_END;
            break;

        // Eats whitespace also
        default:
            c = sb.sbumpc();
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
            child.decode(sb, std::forward<F>(f));
            break;
        }

        case '{':
        {
            item_ = OBJECT;
            state_ = TOKEN_START;
            worker child(this);
            child.decode(sb, std::forward<F>(f));
            break;
        }

        case '}':
            state_ = TOKEN_END;
            break;

        case ':':
            if(item_ != NAME)
                state_ = ERROR;
            break;

        case '"':
        {
            if(parent_ != nullptr && parent_->item() == OBJECT)
            {
                // OBJECT needs NAME then STRING.  Presume NAME mode and
                // toggle to STRING only if NAME mode precedes it
                item_ = item_ == NAME ? STRING : NAME;
            }
            else
                item_ = STRING;

            item_ = parent_ == nullptr ? STRING :
                parent_->item() == OBJECT ? NAME : STRING;
            state_ = TOKEN_START;
            //f(*this);
            break;
        }

        case ' ': break;

        default:
            // Presume a literal
            state_ = TOKEN_START;
            item_ = LITERAL;
            // DEBT: Sloppy, calls again for more character-by-character oriented decode_token
            sb.pubseekoff(-1, ios_base::cur, ios_base::in);
            break;

        case traits::eof():
            break;
    }
}

template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
void decoder::worker<Streambuf, F>::decode(Streambuf& sb, F&& f)
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
            case ERROR:
                return;

            default:    break;
        }
    }
}

template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class Base, class F>
void decoder::decode(estd::detail::basic_istream<Streambuf, Base>& in, F&& f)
{
    worker<Streambuf, F> w;

    w.decode(*in.rdbuf(), std::forward<F>(f));
}

}

}}

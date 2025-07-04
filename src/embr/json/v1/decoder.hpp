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
void decoder::worker<Streambuf, F>::decode_string(context& ctx, F&& f)
{
    streambuf_type& sb = ctx.sb;
    using pointer = const char_type*;

    int idx = 0;
    int_type c;

    while((c = sb.sbumpc()) != '"')
    {
        if ESTD_CPP_CONSTEXPR(17) (options & DECODER_EMIT_CHAR)
            f(*this, item {.ch = static_cast<int>(c)});
        ++idx;
    }

    state_ = TOKEN_END;

    // DEBT: Move this out to consumer
    int pos = sb.pubseekoff(-(idx + 1), estd::ios_base::cur, estd::ios_base::in);     // skip closing '"'
    f(*this, item { .len = idx });
    sb.pubseekpos(pos + idx + 1, estd::ios_base::in);
}
template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
void decoder::worker<Streambuf, F>::decode_number_one(context& ctx, int_type c)
{
    switch(c)
    {
        case ',':
        case ' ':
        case traits::eof():
            // DEBT: Be advised kind of a crummy decimal place adjuster going on here, though
            // it's getting better
            ctx.num.get.finalize(ctx.num.value);
            state_ = TOKEN_END;
            break;

        default:
        {
            ios_base::iostate err = ios_base::goodbit;
            ctx.num.get.get(c, err, ctx.num.value);

            if(err != ios_base::goodbit)
                state_ = ERROR;
            break;
        }
    }
}

template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
void decoder::worker<Streambuf, F>::decode_literal_one(context& ctx, int_type c)
{
    // We're pretty strict about literals.  No match = parse error

    switch(c)
    {
        case ',':
        case ' ':
        case traits::eof():
        {
            // These map to delimiters, so force-feed a delimiter into breadcrumb
            // searcher
            const searcher_type::results r = ctx.literal_searcher.search(0);

            state_ = r == searcher_type::MATCHED ? TOKEN_END : ERROR;
            break;
        }

        default:
        {
            const searcher_type::results r = ctx.literal_searcher.search(c);
            if(r != searcher_type::SEARCHING)
                state_ = ERROR;
            break;
        }
    }
}

template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
void decoder::worker<Streambuf, F>::decode_literal(context& ctx, F&& f)
{
    while(state_ == TOKEN_START)    decode_literal_one(ctx, ctx.sb.sbumpc());

    if(state_ == TOKEN_END)
    {
        const auto id = static_cast<literal_ids>(ctx.literal_searcher.marker_->id);

        f(*this, item { .literal = id });
    }
}


template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
void decoder::worker<Streambuf, F>::decode_number(context& ctx, F&& f)
{
    //streambuf_type& sb = ctx.sb;
    //int_type c;
    //int idx = 0;

    // DEBT: A 'reset' in num_get wouldn't kill us
    //new (&ctx.num_get) num_get_type;
    // DEBT: Init done elsewhere in state machine, but not obvious

    while(state_ == TOKEN_START)    decode_number_one(ctx, ctx.sb.sbumpc());

    if(state_ == TOKEN_END)
    {
        f(*this, item{.number = ctx.num.value});
    }
}


template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
void decoder::worker<Streambuf, F>::decode_token(context& ctx, F&& f)
{
    estd::remove_const_t<char_type> c;
    streambuf_type& sb = ctx.sb;

    switch(item_)
    {
        case NAME:
        case STRING:
            decode_string(ctx, std::forward<F>(f));
            break;

        case NUMBER:
            decode_number(ctx, std::forward<F>(f));
            break;

        case LITERAL:
            decode_literal(ctx, std::forward<F>(f));
            break;

        case OBJECT:
            c = sb.sbumpc();
            if(c == ':')
            {
                worker child(this);
                child.decode(ctx, std::forward<F>(f));
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
void decoder::worker<Streambuf, F>::decode_idle(context& ctx, F&& f)
{
    const int_type c = ctx.sb.sbumpc();

    switch(c)
    {
        case '[':
        {
            item_ = ARRAY;
            state_ = TOKEN_START;
            worker child(this);
            child.decode(ctx, std::forward<F>(f));
            break;
        }

        case '{':
        {
            item_ = OBJECT;
            state_ = TOKEN_START;
            worker child(this);
            child.decode(ctx, std::forward<F>(f));
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
            // Presume a literal or number
            state_ = TOKEN_START;

            if(isdigit(c))
            {
                item_ = NUMBER;
                ctx.num.value = 0;
                new (&ctx.num.get) num_get_type;
                decode_number_one(ctx, c);
            }
            else
            {
                item_ = LITERAL;
                new (&ctx.literal_searcher) searcher_type{json::internal::literals};
                decode_literal_one(ctx, c);
            }
            break;

        case traits::eof():
            break;
    }
}

template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
void decoder::worker<Streambuf, F>::decode(context& ctx, F&& f)
{
    for(;;)
    {
        switch(state_)
        {
            case IDLE:
                decode_idle(ctx, std::forward<F>(f));
                break;

            case TOKEN_START:
                decode_token(ctx, std::forward<F>(f));
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
    using worker_type = worker<Streambuf, F>;
    worker_type w;
    typename worker_type::context ctx{*in.rdbuf()};

    w.decode(ctx, std::forward<F>(f));
}

}

}}

#pragma once

#include <estd/iosfwd.h>
#include "../../internal/breadcrumb.h"

namespace embr { namespace json {

inline namespace v1 {

}

namespace internal {

// https://datatracker.ietf.org/doc/html/rfc8259#section-3
enum literal_ids
{
    ID_TRUE,
    ID_FALSE,
    ID_NULL
};


class decoder_state
{
public:
    // DEBT: Paradigm collision with something like minij::modes
    // "Token" as defined byhttps://datatracker.ietf.org/doc/html/rfc8259#section-2
    // "characters, strings, numbers, and three literal names"
    enum states
    {
        IDLE,
        TOKEN_START,
        TOKEN_END,
        ERROR
    };

    enum items
    {
        OBJECT,
        ARRAY,
        NAME,       ///< aka object key
        NUMBER,
        STRING,
        LITERAL,    // true, false or null
    };

    // DEBT: Displaced by literal_ids - consolidate ... ?
    enum literals
    {
        TRUE,
        FALSE,
        NULL_,
    };

protected:
    states state_{};
    const decoder_state* const parent_;
    items item_;

    using ios_base = estd::ios_base;

public:
    decoder_state(const decoder_state* parent = nullptr) : parent_{parent}    {}

    constexpr states state() const { return state_; }
    constexpr items item() const { return item_; }

    struct descriptor
    {
        union
        {
            int len;
            literal_ids literal;
            float number;
        };
    };

    struct context
    {
        char temp[32];
    };
};

class decoder : public decoder_state
{
    template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf,
        class F>
    struct worker : decoder_state
    {
        using streambuf_type = Streambuf;
        using traits = typename streambuf_type::traits_type;
        using char_type = typename traits::char_type;
        using int_type = typename traits::int_type;
        using pos_type = typename traits::pos_type;
        using item = decoder::descriptor;

        using searcher_type = embr::internal::searcher;

        struct context
        {
            Streambuf& sb;

            union
            {
                searcher_type literal_searcher;
            };
        };

        // If true, don't assume gptr is available.
        static constexpr bool is_locking = true;

        // DEBT: Break these down into decode_one so that we can completely
        // avoid blocking

        // Since F confuses with char_type, don't overload the names
        void decode_literal_one(context&, char_type c);
        void decode_number_one(context&, char_type c);

        void decode(context& sb, F&& f);

        void decode_idle(context& sb, F&& f);
        void decode_literal(context& sb, F&& f);
        void decode_number(context& sb, F&& f);
        void decode_string(context& sb, F&& f);
        void decode_token(context& sb, F&& f);

        constexpr explicit worker(const worker* parent = nullptr) :
            decoder_state(parent)
        {}
    };

public:
    template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class Base, class F>
    void decode(estd::detail::basic_istream<Streambuf, Base>& in, F&& f);
};

}

}}

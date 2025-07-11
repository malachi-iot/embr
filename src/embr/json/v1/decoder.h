#pragma once

#include <estd/internal/locale/ctype.h>
#include <estd/internal/locale/num_get.h>
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
        TOKEN_MIDDLE,

        // UNUSED - prep to clean DEBT from OBJECT parse
        TOKEN_PHASE1 = TOKEN_MIDDLE,
        TOKEN_PHASE2,

        TOKEN_END,
        ERROR
    };

    enum object_states
    {
        OBJECT_START,
        OBJECT_NAME,
        OBJECT_VALUE
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

    using this_type = decoder_state;

protected:
    states state_ : 8;
    items item_ : 8;
    const decoder_state* const parent_;

    using ios_base = estd::ios_base;

public:
    decoder_state(const decoder_state* parent) :
        state_{IDLE},
        parent_{parent}
    {}

    constexpr states state() const { return state_; }
    constexpr items item() const { return item_; }
    constexpr const this_type* parent() const { return parent_; }
    constexpr bool has_parent() const { return parent_ != nullptr; }

    struct descriptor
    {
        union
        {
            int len;
            literal_ids literal;
            double number;
            int ch;
        };

        template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class F>
        void str_op(Streambuf& sb, F&& f) const;

        template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf>
        estd::basic_string_view<typename Streambuf::char_type> str(Streambuf& sb) const;

        template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class Char>
        int str(Streambuf& sb, Char*) const;

        template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class Base>
        constexpr estd::basic_string_view<typename Streambuf::char_type> str(
            estd::detail::basic_istream<Streambuf, Base>& in) const
        {
            return str(*in.rdbuf());
        }

        template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class Base, typename Char>
        int str(
            estd::detail::basic_istream<Streambuf, Base>& in, Char* s) const
        {
            return str(*in.rdbuf(), s);
        }
    };
};

enum decoder_options
{
    DECODER_NONE            = 0x00,
    /// Emit character-by-character functor calls when observing strings
    DECODER_EMIT_CHAR       = 0x01,
    /// Favor sgetc over sbumpc (NOT READY)
    DECODER_SGETC           = 0x02,
    DECODER_DEFAULT         = DECODER_EMIT_CHAR,
};

ESTD_FLAGS(decoder_options)

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
        using nonconst_char_type = estd::remove_const_t<char_type>;

        using searcher_type = embr::breadcrumb::v1::searcher;

        using locale_type = estd::internal::default_locale;
        using num_get_type = estd::iterated::num_get<10, char_type, locale_type>;

        // DEBT: https://github.com/malachi-iot/estdlib/issues/132
        using ctype = estd::ctype<nonconst_char_type, locale_type>;

        // FIX: Somehow in c++14 this flips out and kills the linker
        static constexpr decoder_options options = DECODER_DEFAULT;

        struct context
        {
            Streambuf& sb;
            int_type ch;

            void bump() { ch = sb.sbumpc(); }

            union
            {
                searcher_type literal_searcher;
                struct
                {
                    num_get_type get;
                    double value;
                }   num;
            };
        };

        // Dormant
        union state
        {
            int array_index;
            object_states object_state;
        };

        // DEBT: Break these down into decode_one so that we can completely
        // avoid blocking

        void decode_whitespace(context&);

        // Since F confuses with char_type, don't overload the names
        void decode_literal_one(context&, int_type c);
        void decode_number_one(context&, int_type c);

        void decode(context&, F&& f);

        void decode_array(context&, F&& f);
        void decode_idle(context&, F&& f);
        void decode_literal(context&, F&& f);
        void decode_number(context&, F&& f);
        void decode_object(context&, F&& f);
        void decode_string(context&, F&& f);
        void decode_token(context&, F&& f);

        constexpr explicit worker(const worker* parent = nullptr) :
            decoder_state(parent)
        {}
    };

public:
    explicit decoder() : decoder_state(nullptr)   {}

    template <decoder_options o = DECODER_DEFAULT, ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Streambuf, class Base, class F>
    static void decode(estd::detail::basic_istream<Streambuf, Base>& in, F&& f);
};

}

}}

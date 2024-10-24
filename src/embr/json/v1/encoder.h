#pragma once

#include <estd/iosfwd.h>

namespace embr { namespace json {

inline namespace v1 {

namespace options {

// NOTE: For valid JSON, use_doublequotes() *must* be true
// NOTE: For valid JSON, no actual tab character can be in tab()

// use_spaces() describes whitespace between key/value elements,
// and does not relate to tab()

struct base
{
    static ESTD_CPP_CONSTEXPR_RET const char* tab() { return nullptr; }
    static ESTD_CPP_CONSTEXPR_RET bool use_doublequotes() { return true; }
};

struct full : base
{
    static ESTD_CPP_CONSTEXPR_RET const char* tab() { return "  "; }
    bool use_eol() { return true; }
    bool use_spaces() { return true; }
    bool brace_on_newline() { return true; }
};

struct lean : base
{
    bool use_eol() { return false; }
    bool use_spaces() { return false; }
    bool brace_on_newline() { return false; }
};

}



template <class TOptions = options::lean>
class encoder : TOptions
{
    // DEBT: Feels like we can optimize 'has_items'
    // DEBT: Consider a feature flag to reduce all this to 16-bit
    // (would then max out at 8 levels instead of 16)
    struct
    {
        // tracks whether a certain nested level has any items
        uint32_t has_items_: 16;
        uint32_t level_: 4;
        uint32_t in_array_: 1;
    };

public:
    typedef TOptions options_type;

    ESTD_CPP_CONSTEXPR_RET encoder() : has_items_(0), level_(0), in_array_(0)
    {

    }

    template <class TStreambuf, class TBase>
    inline void do_tabs(estd::detail::basic_ostream <TStreambuf, TBase>& out)
    {
        if (options_type::tab() != NULLPTR)
        {
            for (unsigned i = level_; i > 0; --i)
                out << options_type::tab();
        }
    }

    template <class TStreambuf, class TBase>
    inline void do_eol(estd::detail::basic_ostream <TStreambuf, TBase>& out);

    ESTD_CPP_CONSTEXPR_RET char quote(bool key = true) const
    {
        return options_type::use_doublequotes() ? '"' : '\'';
    }

    bool has_items() const { return has_items_ >> level_; }
    bool in_array() const { return in_array_; }

    void set_has_items()
    {
        has_items_ |= 1 << level_;
    }

    // clear out indicator that this level has items
    // (do this when moving back up a level)
    void clear_has_items()
    {
        has_items_ &= ~(1 << level_);
    }

    // Prints comma and eol if items preceded this
    template <class TStreambuf, class TBase>
    void do_has_items(estd::detail::basic_ostream <TStreambuf, TBase>& out)
    {
        if (has_items())
        {
            out << ',';
            do_eol(out);
        }
        else
            set_has_items();
    }

    template <class TStreambuf, class TBase>
    void begin(estd::detail::basic_ostream <TStreambuf, TBase>& out)
    {
        do_tabs(out);
        do_has_items(out);

        out << '{';
        do_eol(out);
        ++level_;
    }

    template <class TStreambuf, class TBase>
    void add_key(estd::detail::basic_ostream <TStreambuf, TBase>& out, const char* key)
    {
        do_has_items(out);

        do_tabs(out);

        out << quote() << key << quote() << ':';

        if (options_type::use_spaces()) out << ' ';
    }

    template <class TStreambuf, class TBase>
    void begin(estd::detail::basic_ostream <TStreambuf, TBase>& out, const char* key)
    {
        add_key(out, key);

        if (options_type::brace_on_newline())
        {
            do_eol(out);
            do_tabs(out);
        }

        out << '{';
        do_eol(out);

        ++level_;
    }

    template <class TStreambuf, class TBase>
    void array(estd::detail::basic_ostream<TStreambuf, TBase>& out, const char* key)
    {
        in_array_ = 1;

        add_key(out, key);

        out << '[';
        ++level_;
    }

    // TODO: deduce what kind of quote would be better by inspecting value, or
    // at least provide a compile time hint
    template <class TStreambuf, class TBase>
    void raw(estd::detail::basic_ostream<TStreambuf, TBase>& out, const char* value)
    {
        out << quote(false) << value << quote(false);
    }

    template <class Streambuf, class Base>
    void raw(estd::detail::basic_ostream<Streambuf, Base>& out, int value)
    {
        out << value;
    }

    template <class Streambuf, class Base, typename T>
    void array_item(estd::detail::basic_ostream<Streambuf, Base>& out, T value)
    {
        if (has_items())
        {
            out << ',';
            if (options_type::use_spaces()) out << ' ';
        }
        else
            set_has_items();

        raw(out, value);
    }

    template <class TStreambuf, class TBase>
    void end_array(estd::detail::basic_ostream <TStreambuf, TBase>& out)
    {
        out << ']';
        --level_;
        in_array_ = 0;
    }

    template <class TStreambuf, class TBase>
    void end(estd::detail::basic_ostream<TStreambuf, TBase>& out)
    {
        clear_has_items();

        if (in_array())
        {
            end_array(out);
            return;
        }

        --level_;

        do_eol(out);
        do_tabs(out);

        out << '}';
    }

    template <class TStreambuf, class TBase, typename T>
    void add(estd::detail::basic_ostream<TStreambuf, TBase>& out, const char* key, T value)
    {
        add_key(out, key);
        raw(out, value);
    }

    // Interrogations.  Not used in normal operation
    
    /// What our current nest level is
    ESTD_CPP_CONSTEXPR_RET uint16_t level() const { return level_; }
};


}

}}

#pragma once

#include <estd/cstdint.h>
#include <estd/type_traits.h>

#include "internal/word.h"

#if FEATURE_EMBR_WORD_STRICTNESS
#include "enum_mask.h"
#endif

#include "platform/guard-in.h"

namespace embr {

#if __cplusplus >= 201103L
template <class T, class U>
constexpr T narrow_cast(U&& u) noexcept
{
    typedef internal::narrow_cast<
        typename estd::remove_reference<U>::type,
        typename estd::remove_reference<T>::type> caster_type;

    return caster_type::cast(std::move(u));
}
#endif


}

namespace estd {

template <size_t bits, embr::word_strictness strict>
struct numeric_limits<embr::word<bits, false, strict> >
{
    typedef embr::word<bits, false, strict> word_type;
    typedef typename word_type::type type;

    static ESTD_CPP_CONSTEXPR_RET type min() { return 0; }
    static ESTD_CPP_CONSTEXPR_RET type max() { return word_type::mask(); }
    static CONSTEXPR int digits = bits;
};

// Helper macro just for this header
#pragma push_macro("EMBR_WORD_ARITHMETIC")
// FIX: This needs the enable_if_t<bool, bool> = true treatment
#if FEATURE_EMBR_WORD_STRICTNESS
#define EMBR_WORD_ARITHMETIC ,typename Enabled = estd::enable_if_t<any<s, word_strictness::arithmetic>()>
#else
#define EMBR_WORD_ARITHMETIC
#endif

#if __cplusplus >= 201103L
// Converts word back to requested integer with a compile time guard against
// narrowing conversion
template <class Int, size_t bits, bool is_signed, embr::word_strictness strict,
typename enable_if<
#if FEATURE_EMBR_WORD_STRICTNESS
    (!embr::any<strict, embr::word_strictness::narrowing>()) ||
#endif
    (estd::numeric_limits<Int>::max() >= estd::numeric_limits<embr::word<bits> >::max()), bool>::type = true>
#else
// For c++03 is is just a noop for forward compatibility.
// DEBT: If need be, a c++03 version could be made using the UINT_MAX style defines
template <class Int, size_t bits, bool is_signed, embr::word_strictness strict>
#endif
ESTD_CPP_CONSTEXPR_RET Int to_integer(embr::word<bits, is_signed, strict> w)
{
    return w.cvalue();
}

}
namespace embr {



// Auto-sizing word
template <size_t bits, bool is_signed, word_strictness strict, typename T>
class word : public internal::word_base<bits, is_signed, strict, T>
{
    typedef internal::word_base<bits, is_signed, strict, T> base_type;

#if FEATURE_EMBR_WORD_STRICTNESS
    typedef internal::enum_mask<word_strictness, strict> h;
#endif

#if __cpp_alias_templates
    /// enable if specified bits are less than our equal to bits
    /// this word can handle
    template <size_t b>
    using bits_fit = estd::enable_if_t<(b <= bits), bool>;
#endif

public:
    typedef typename base_type::type type;

public:
    ESTD_CPP_DEFAULT_CTOR(word)
    ESTD_CPP_CONSTEXPR_RET word(const type& value) : base_type(value) {}
#ifdef __cpp_rvalue_references
    constexpr word(type&& value) : base_type(std::forward<type>(value)) {}
#endif

    // DEBT: This can be optimized by skipping any masking altogether when the source
    // word has masking employed - we are guaranteed in that case to receive a pristine
    // bit limited word which we know already fits into our own
#ifdef FEATURE_CPP_DEFAULT_TARGS
    template <size_t incoming_bits, bits_fit<incoming_bits> = true>
    constexpr
#else
    template <size_t incoming_bits>
    inline
#endif
    word(const word<incoming_bits>& copy_from) :
#if FEATURE_EMBR_WORD_STRICTNESS_DISABLED
        // base_type handles all masking now.  Keeping around just incase I forgot something
        base_type(h::template all<h::e::masking>() ?
            base_type::mask(copy_from.cvalue()) :
            copy_from.cvalue())
#else
        base_type(copy_from.cvalue())
#endif
    {}

    //type& value() { return value_; }

    // DEBT: Filter this by strictness flags, i.e. we don't want uint16_t automatically
    // assignable to 11-bit words
    word& operator=(type value)
    {
        return * (new (this) word(value));      // NOLINT
    }

    word& operator++()
    {
#if FEATURE_EMBR_WORD_STRICTNESS
        // EXPERIMENTAL TODO: - not complete, roll over "early" if set
        if(h::template any<word_strictness::overflow2>()) {}
#endif

        ++base_type::value_;
        return *this;
    }

    template <size_t bits_rhs
#ifdef FEATURE_CPP_DEFAULT_TARGS
        , bits_fit<bits_rhs> = true
#endif
        >
    word& operator +=(const word<bits_rhs, is_signed, strict>& r)
    {
        base_type::value_ += r.cvalue();
        return *this;
    }

    template <size_t bits_rhs
#ifdef FEATURE_CPP_DEFAULT_TARGS
        //, estd::enable_if_t<(bits_rhs <= bits), bool> = true
        , bits_fit<bits_rhs> = true
#endif
        >
    word& operator -=(word<bits_rhs> r)
    {
        base_type::value_ -= r.cvalue();
        return *this;
    }


    template <size_t bits_rhs, word_strictness strict2
#ifdef FEATURE_CPP_DEFAULT_TARGS
        , bits_fit<bits_rhs> = true
#endif
        >
    inline word& operator &=(word<bits_rhs, is_signed, strict2> r)
    {
        base_type::value_ &= r.cvalue();
        return *this;
    }

    template <class Int>
    inline word& operator <<=(Int r)
    {
        // DEBT: If strict masking is on, apply mask here - and consider an overflow detect feature
        base_type::value_ <<= r;
        return *this;
    }


    template <size_t bits_rhs, word_strictness strict2
#ifdef FEATURE_CPP_DEFAULT_TARGS
        , bits_fit<bits_rhs> = true
#endif
        >
    word& operator |=(const word<bits_rhs, is_signed, strict2>& r)
    {
        base_type::value_ |= r.cvalue();
        return *this;
    }


    template <size_t bits_rhs, word_strictness strict2
#ifdef FEATURE_CPP_DEFAULT_TARGS
        , bits_fit<bits_rhs> = true
#endif
    >
    word& operator ^=(const word<bits_rhs, is_signed, strict2>& r)
    {
        base_type::value_ ^= r.cvalue();
        return *this;
    }

    template <size_t bits_rhs
#ifdef FEATURE_CPP_DEFAULT_TARGS
        , bits_fit<bits_rhs> = true
#endif
        >
    ESTD_CPP_CONSTEXPR_RET word operator +(word<bits_rhs> r) const
    {
        return word((type) (base_type::value_ + r.value()));
    }

    template <size_t bits_rhs
#ifdef FEATURE_CPP_DEFAULT_TARGS
        , bits_fit<bits_rhs> = true
#endif
    >
    ESTD_CPP_CONSTEXPR_RET word operator *(word<bits_rhs> r) const
    {
        return word((type) (base_type::value_ * r.value()));
    }

    template <size_t bits_rhs
#ifdef FEATURE_CPP_DEFAULT_TARGS
        , bits_fit<bits_rhs> = true
#endif
        >
    ESTD_CPP_CONSTEXPR_RET word operator &(word<bits_rhs> r) const
    {
        return word((type) (base_type::value_ & r.value()));
    }

    // DEBT: May be an issue, should only flip active bits
    ESTD_CPP_CONSTEXPR_RET word operator ~() const { return word(~base_type::value_); }
};


// Since auto conversion doesn't happen on template deduction, we have to manually
// do these overloads.  At least, that's what's implied here:
// https://stackoverflow.com/questions/22902698/with-template-constructor-template-argument-deduction-substitution-failed-why
// +++

template <size_t bits>
ESTD_CPP_CONSTEXPR_RET word<bits> operator &(word<bits> l, typename word<bits>::type r)
{
    return l & word<bits>(r);
}

template <size_t bits, bool is_signed, word_strictness s
#if FEATURE_EMBR_WORD_STRICTNESS
    , estd::enable_if_t<
        any<s, word_strictness::arithmetic>(), bool> = true
#endif
    >
inline word<bits, is_signed, s>& operator +=
    (word<bits, is_signed, s>& l, typename word<bits, is_signed>::type r)
{
    return l += word<bits, is_signed, s>(r);
}


template <size_t bits, bool is_signed, word_strictness s
#if FEATURE_EMBR_WORD_STRICTNESS
    , estd::enable_if_t<
        any<s, word_strictness::arithmetic>(), bool> = true
#endif
    >
ESTD_CPP_CONSTEXPR_RET word<bits, is_signed, s> operator +(
    word<bits, is_signed, s> l, typename word<bits, is_signed, s>::type r)
{
    return l + word<bits, is_signed, s>(r);
}


template <size_t bits, bool is_signed, word_strictness s EMBR_WORD_ARITHMETIC>
ESTD_CPP_CONSTEXPR_RET word<bits, is_signed, s> operator *(
    word<bits, is_signed, s> l, typename word<bits, is_signed, s>::type r)
{
    return l * word<bits, is_signed, s>(r);
}

template <size_t bits, bool is_signed, word_strictness s, typename Int EMBR_WORD_ARITHMETIC>
ESTD_CPP_CONSTEXPR_RET Int operator *(
    word<bits, is_signed, s> l, Int r)
{
    return l.value() * r;
}

template <size_t bits, typename Int>
ESTD_CPP_CONSTEXPR_RET word<bits> operator <<(word<bits> l, Int r)
{
    return word<bits>(l.cvalue() << r);
}

template <size_t bits, typename Int>
ESTD_CPP_CONSTEXPR_RET word<bits> operator >>(word<bits> l, Int r)
{
    return word<bits>(l.cvalue() >> r);
}


template <size_t bits, bool is_signed, word_strictness s, typename T, typename Int>
ESTD_CPP_CONSTEXPR_RET bool operator !=(word<bits, is_signed, s, T> l, Int r)
{
    return l.cvalue() != r;
}

template <size_t bits, bool is_signed, word_strictness s, typename T, typename Int>
ESTD_CPP_CONSTEXPR_RET bool operator ==(word<bits, is_signed, s, T> l, Int r)
{
    return l.cvalue() == r;
}

// ---

/*
template <size_t bits>
constexpr bool operator ==(const word<bits>& lhs, const word<bits>& rhs)
{
    return lhs._v() == rhs._v();
} */


}

#pragma pop_macro("EMBR_WORD_ARITHMETIC")

#include "platform/guard-out.h"

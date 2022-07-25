#pragma once

#include <estd/cstdint.h>
#include <estd/type_traits.h>

#include "internal/word.h"

#include "enum_mask.h"

#include "platform/guard-in.h"

namespace embr {

template <word_strictness s>
using strictness_helper = internal::enum_mask<word_strictness, s>;

/// Determines if any 's' bits are present in 'v'
template <word_strictness v, word_strictness... s>
constexpr bool any()
{
    return strictness_helper<v>::template any<s...>();
}

/// Determines if all 's' bits are present in 'v'
template <word_strictness v, word_strictness... s>
constexpr bool all()
{
    return strictness_helper<v>::template all<s...>();
}

template <class T, class U>
constexpr T narrow_cast(U&& u) noexcept
{
    typedef internal::narrow_cast<
        typename estd::remove_reference<U>::type,
        typename estd::remove_reference<T>::type> caster_type;

    return caster_type::cast(std::move(u));
}


}

namespace estd {

template <size_t bits, embr::word_strictness strict>
struct numeric_limits<embr::word<bits, false, strict> >
{
    typedef embr::word<bits, false, strict> word_type;
    typedef typename word_type::type type;

    static constexpr type min() { return 0; }
    static constexpr type max() { return word_type::mask(); }
};

// Converts word back to requested integer with a compile time guard against
// narrowing conversion
template <class TInt, size_t bits, bool is_signed, embr::word_strictness strict, typename Enabled =
typename enable_if<
    (!embr::any<strict, embr::word_strictness::narrowing>()) ||
    (estd::numeric_limits<TInt>::max() >= estd::numeric_limits<embr::word<bits> >::max())>::type >
constexpr TInt to_integer(embr::word<bits, is_signed, strict> w)
{
    return w.cvalue();
}

}
namespace embr {



// Auto-sizing word
template <size_t bits, bool is_signed, word_strictness strict>
class word : public internal::word_base<bits, is_signed, strict>
{
    typedef internal::word_base<bits, is_signed, strict> base_type;

    typedef internal::enum_mask<word_strictness, strict> h;

public:
    typedef typename base_type::type type;

protected:

    type value_;

public:
    word() = default;
    constexpr word(const type& value) : value_{
        any<strict, word_strictness::masking>() ? base_type::mask(value) : value} {}
    constexpr word(type&& value) : value_{
        any<strict, h::e::masking>() ? base_type::mask(value) : value} {}

    // DEBT: This can be optimized by skipping any masking altogether when the source
    // word has masking employed - we are guaranteed in that case to receive a pristine
    // bit limited word which we know already fits into our own
    template <size_t incoming_bits, class Enabled =
        typename estd::enable_if<(incoming_bits <= bits)>::type>
    constexpr word(const word<incoming_bits>& copy_from) :
        value_{h::template all<h::e::masking>() ?
            base_type::mask(copy_from.cvalue()) :
            copy_from.cvalue()}
    {}

    //type& value() { return value_; }
    constexpr const type& value() const { return value_; }
    constexpr const type& cvalue() const { return value_; }

    // DEBT: Filter this by strictness flags, i.e. we don't want uint16_t automatically
    // assignable to 11-bit words
    word& operator=(type value)
    {
        return * (new (this) word(value));
    }

    template <size_t bits_rhs, class Enabled =
        typename estd::enable_if<(bits_rhs <= bits)>::type>
    inline word& operator +=(word<bits_rhs> r)
    {
        value_ += r.cvalue();
        return *this;
    }

    template <size_t bits_rhs, class Enabled =
        typename estd::enable_if<(bits_rhs <= bits)>::type>
    inline word& operator -=(word<bits_rhs> r)
    {
        value_ -= r.cvalue();
        return *this;
    }


    template <size_t bits_rhs, class Enabled =
        typename estd::enable_if<(bits_rhs <= bits)>::type>
    inline word& operator &=(word<bits_rhs> r)
    {
        value_ &= r.cvalue();
        return *this;
    }

    template <class TInt>
    inline word& operator <<=(TInt r)
    {
        // DEBT: If strict masking is on, apply mask here - and consider an overflow detect feature
        value_ <<= r;
        return *this;
    }


    template <size_t bits_rhs, class Enabled =
        typename estd::enable_if<(bits_rhs <= bits)>::type>
    inline word& operator |=(word<bits_rhs> r)
    {
        value_ |= r.cvalue();
        return *this;
    }


    template <size_t bits_rhs, class Enabled =
        typename estd::enable_if<(bits_rhs <= bits)>::type>
    constexpr word operator +(word<bits_rhs> r) const
    {
        return word{(type) (value_ + r.value())};
    }

    template <size_t bits_rhs, class Enabled =
        typename estd::enable_if<(bits_rhs <= bits)>::type>
    constexpr word operator &(word<bits_rhs> r) const
    {
        return word{(type) (value_ & r.value())};
    }

    constexpr word operator ~() const { return word{~value_}; }
};


// Since auto conversion doesn't happen on template deduction, we have to manually
// do these overloads.  At least, that's what's implied here:
// https://stackoverflow.com/questions/22902698/with-template-constructor-template-argument-deduction-substitution-failed-why
// +++

template <size_t bits>
constexpr word<bits> operator &(word<bits> l, typename word<bits>::type r)
{
    return l & word<bits>{r};
}

template <size_t bits, bool is_signed, word_strictness s,
    typename Enabled = estd::enable_if_t<
        any<s, word_strictness::arithmetic>()> >
inline word<bits, is_signed, s>& operator +=
    (word<bits, is_signed, s>& l, typename word<bits, is_signed>::type r)
{
    return l += word<bits, is_signed, s>{r};
}


template <size_t bits, bool is_signed, word_strictness s,
    typename Enabled = estd::enable_if_t<
        any<s, word_strictness::arithmetic>()> >
constexpr word<bits, is_signed, s> operator +(
    word<bits, is_signed, s> l, typename word<bits, is_signed, s>::type r)
{
    return l + word<bits, is_signed, s>{r};
}

template <size_t bits, typename TInt>
constexpr word<bits> operator <<(word<bits> l, TInt r)
{
    return word<bits>{l.cvalue() << r};
}

template <size_t bits, typename TInt>
constexpr word<bits> operator >>(word<bits> l, TInt r)
{
    return word<bits>{l.cvalue() >> r};
}


template <size_t bits, bool is_signed, word_strictness s, typename TInt>
constexpr bool operator !=(word<bits, is_signed, s> l, TInt r)
{
    return l.cvalue() != r;
}

template <size_t bits, bool is_signed, word_strictness s, typename TInt>
constexpr bool operator ==(word<bits, is_signed, s> l, TInt r)
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

#include "platform/guard-out.h"

#pragma once

#include <estd/algorithm.h>
#include <estd/flags.h>
#include <estd/type_traits.h>
#include <estd/internal/units/fwd.h>

#include <embr/fwd/type_from_bits.h>
#include <embr/internal/type_from_bits.h>

#include "fwd.h"
#include "packed.h"

#include <estd/internal/macro/push.h>

// EXPERIMENTAL
// Working well so far, just not noticeably quicker
#define FEATURE_EMBR_DSP_FP_PERMISSIVE_CONVERSION 0

namespace embr { namespace dsp { inline namespace v1 {

// Guidance from:
// https://stackoverflow.com/questions/10067510/fixed-point-arithmetic-in-c-programming
// https://en.m.wikipedia.org/wiki/Fixed-point_arithmetic
// https://stackoverflow.com/questions/53915557/how-do-i-parse-a-twos-complement-fixed-point-number-from-a-primitive-integer-ty

// Friends with, but slightly differnt than:
// https://en.cppreference.com/w/cpp/numeric/math/frexp

// FIX: Specifying endian twice makes no sense
// DEBT: Not sure why we have to specify 'packed' here but if we don't kern table code complains about fp12_4
template <unsigned exponent_, unsigned mantissa_, fixed_point_options o>
struct __attribute__ ((packed)) fixed_point<exponent_, mantissa_, o, estd::endian::native> :
    packed_word<(o & FP_SIGNED) ? PACKED_WORD_SIGNED : PACKED_WORD_DEFAULT, exponent_, mantissa_>
{
    static constexpr unsigned exponent = exponent_;
    static constexpr unsigned mantissa = mantissa_;

    using base_type = packed_word<(o & FP_SIGNED) ? PACKED_WORD_SIGNED : PACKED_WORD_DEFAULT, exponent_, mantissa_>;
    using base_type::channel;
    using base_type::v_;
    using typename base_type::signed_type;
    using typename base_type::unsigned_type;
    using typename base_type::value_type;
    using this_type = fixed_point;

    static constexpr bool is_implicit = o & FP_IMPLICIT;
    static constexpr bool is_signed = o & FP_SIGNED;
    static constexpr unsigned mantissa_max = 1 << mantissa;
    using promoted_type = type_from_bits_t<1 + exponent + mantissa, is_signed>;

    using relaxed_t = estd::units::v1::relaxed_narrow_t;

    constexpr fixed_point() = default;

    template <class T>
    constexpr fixed_point(relaxed_t, const T& v) :
        base_type{static_cast<value_type>(v)}
    {}

#if __cpp_consteval
    // 10MAY26 - I don't like how this and the following constructor behave differently despite nearly
    // identical signatures.  Since it's guarded by is_implicit, I think it's OK - implicit carries
    // "magic" behaviors
    consteval fixed_point(double v) requires is_implicit :
        base_type(from_ll(v))
    {}
#endif

    constexpr explicit fixed_point(value_type v) :
        base_type{v}
    {}

    // EXPERIMENTAL
    // Alternative to above explicit raw ininitializer
    constexpr fixed_point(estd::in_place_t, value_type v) :
        base_type{v}
    {}

    constexpr fixed_point(const fixed_point&) = default;

    template <unsigned exponent2, unsigned mantissa2>
    constexpr value_type convert_ll(const fixed_point<exponent2, mantissa2, o>& convert_from)
    {
        // DEBT: Looks like we might want a convenience shifter helper for this
        constexpr int d = mantissa - mantissa2;
        if constexpr(d == 0)
            return convert_from.value();
        else if constexpr(d > 0)
            return convert_from.value() << d;
        else
            return convert_from.value() >> -d;
    }


#if FEATURE_EMBR_DSP_FP_PERMISSIVE_CONVERSION
    // EXPERIMENTAL
    // Implicit conversion - silent precision loss permitted
    template <unsigned exponent2, unsigned mantissa2,
        bool Implicit = is_implicit, estd::enable_if_t<Implicit, int> = 0>
    constexpr fixed_point(const fixed_point<exponent2, mantissa2, o>& copy_from) :
        base_type(convert_ll(copy_from))
    {

    }
#endif

    ///
    /// @brief from
    /// @param v intrinsic such as a float, int, etc
    /// @return
    ///
    template <class Numeric>
    static constexpr auto from_ll(const Numeric& v) ->
        estd::enable_if_t<estd::is_arithmetic<Numeric>::value, value_type>
    {
        // NOTE: constexpr if largely unnecessary - but it is easy enough to
        // give compiler every opportunity to use a bit shift instead of a multiply
        if constexpr (estd::numeric_limits<Numeric>::is_integer &&
            !estd::numeric_limits<Numeric>::is_signed)
            return v << mantissa;
        else
            return v * mantissa_max;
    }

    template <class Numeric>
    static constexpr auto from(const Numeric& v) ->
        estd::enable_if_t<estd::is_arithmetic<Numeric>::value, fixed_point>
    {
        return fixed_point(from_ll(v));
    }

    ///
    /// @brief from
    /// @param v
    /// @param native when true, treat numeric as fixed_point already in raw numeric form,
    ///         otherwise treat it as a normal intrinsic float, int etc.
    /// @return
    ///
    template <class Numeric>
    static constexpr this_type from(Numeric v, bool already_fp)
    {
        return already_fp ? this_type{v} : from(v);
    }

    static ESTD_CPP_CONSTEVAL this_type zero() { return this_type{ 0 }; }

    /*
    constexpr fixed_point(value_type n, value_type d) :
        value_{n << numerator | d}
    {}  */

    constexpr unsigned_type exp_u() const
    {
        return base_type::template channel<0, unsigned_type>();
    }

    constexpr value_type exp() const
    {
        // DEBT: I think we can just return channel<0, value_type> right?
        if constexpr(is_signed)
            return base_type::template channel<0, signed_type>();
        else
            return exp_u();
    }

    constexpr unsigned_type man() const
    {
        return base_type::template channel<1>();
    }

    template <class Float>
    constexpr Float man_f() const
    {
        // TODO: See if this optimizes down to an actual * 1/1<<mantissa - if not,
        // whip one up here
        return static_cast<Float>(man()) / (1 << mantissa);
    }

    template <class Float>
    constexpr Float as() const { return man_f<Float>() + exp(); }

    template <unsigned exp2, fixed_point_options o2>
    this_type& operator+=(const fixed_point<exp2, mantissa, o2>& addend)
    {
        v_ += addend.value();
        return *this;
    }

    template <unsigned exp2, fixed_point_options o2>
    this_type& operator-=(const fixed_point<exp2, mantissa, o2>& addend)
    {
        v_ -= addend.value();
        return *this;
    }

    constexpr this_type& operator>>=(int v)
    {
        base_type::v_ >>= v;
        return *this;
    }

    template <class Integer>
    ESTD_CPP_CONSTEXPR(14) auto operator*=(const Integer& rhs) ->
        estd::enable_if_t<estd::numeric_limits<Integer>::is_integer, fixed_point&>
    {
        v_ *= rhs;
        return *this;
    }

    template <class Integer>
    ESTD_CPP_CONSTEXPR(14) auto operator/=(const Integer& rhs) ->
        estd::enable_if_t<estd::numeric_limits<Integer>::is_integer, fixed_point&>
    {
        v_ /= rhs;
        return *this;
    }

    template <unsigned exp2, fixed_point_options o2>
    this_type& operator*=(const fixed_point<exp2, mantissa, o2>& v)
    {
        // Not using *= so that type promotion has a chance to occur
        v_ = static_cast<promoted_type>(v_ * v.value()) >> mantissa;
        return *this;
    }

    /*
    template <unsigned exp2, unsigned mantissa2, fixed_point_options o2>
    this_type& operator*=(const fixed_point<exp2, mantissa2, o2>& v)
    {
        promoted_type temp = v_;
        temp *= v.value();

        // Not using *= so that type promotion has a chance to occur
        if constexpr(mantissa2 < mantissa)
        {
            // This block kinda works
            constexpr unsigned delta = mantissa - mantissa2;

            if constexpr(delta > mantissa)
                v_ = temp << (delta - mantissa);
            else
                v_ = temp >> (mantissa - delta);
        }
        else
        {
            // This block doesn't work so well
            constexpr unsigned delta = mantissa2 - mantissa;

            if constexpr(delta > mantissa)
                v_ = temp >> delta;
            else
                v_ = temp << (mantissa - delta);
        }

        return *this;
    }   */

    template <unsigned exp2, fixed_point_options o2>
    this_type& operator/=(const fixed_point<exp2, mantissa, o2>& v)
    {
        // DEBT: promoted_type always ends up signed, which is harmless so far, but incorrect
        // and annoying.  Oftentimes built in integral promotion will already handle things
        // (i.e. v_ = uint16_t automatically promoting to 32-bit int on ESP32) but this
        // cast makes sure that we ALWAYS promote
        v_ = static_cast<value_type>((static_cast<promoted_type>(v_) << mantissa) / v.value());
        return *this;
    }

    constexpr fixed_point<exponent, mantissa, o | FP_SIGNED> operator-() const
    {
        return { relaxed_t{}, -v_ };
    }

    constexpr bool operator==(const this_type& compare_to) const
    {
        return compare_to.value() == base_type::v_;
    }

    template <unsigned exp2, fixed_point_options o2>
    constexpr bool operator==(const fixed_point<exp2, mantissa, o2>& compare_to) const
    {
        return compare_to.man() == man() && compare_to.exp() == exp();
    }

    /*
    template <unsigned exp2, unsigned man2, fixed_point_options o2>
    constexpr bool operator==(const fixed_point<exp2, man2, o2>& compare_to)
    {
        return compare_to.man() == man() && compare_to.exp() == exp();
    }   */

    // NOTE: Deviates from standard C++ behavior in that auto-promotion to int doesn't happen
    // TODO: See if we can crtp-consolidate this with estd::units
    template <class Integer>
    friend constexpr auto operator/(const fixed_point& lhs, Integer rhs) ->
        estd::enable_if_t<estd::numeric_limits<Integer>::is_integer, fixed_point>
    {
        return { relaxed_t{}, lhs.value() / rhs };
    }

    // NOTE: Deviates from standard C++ behavior in that auto-promotion to int doesn't happen
    template <class Integer>
    friend constexpr auto operator*(const fixed_point& lhs, Integer rhs) ->
        estd::enable_if_t<estd::numeric_limits<Integer>::is_integer, fixed_point>
    {
        return { relaxed_t{}, lhs.value() * rhs };
    }

    friend constexpr fixed_point operator-(const fixed_point& lhs, const fixed_point& rhs)
    {
        return { relaxed_t{}, lhs.value() - rhs.value() };
    }

    friend constexpr fixed_point operator+(const fixed_point& lhs, const fixed_point& rhs)
    {
        return { relaxed_t{}, lhs.value() + rhs.value() };
    }

    template <class Numeric>
    friend constexpr auto operator-(Numeric lhs, const fixed_point& rhs) ->
        estd::enable_if_t<is_implicit && estd::is_arithmetic<Numeric>::value, fixed_point>
    {
        return from(lhs) - rhs;
    }

    template <class Numeric>
    friend constexpr auto operator+(Numeric lhs, const fixed_point& rhs) ->
        estd::enable_if_t<is_implicit && estd::is_arithmetic<Numeric>::value, fixed_point>
    {
        return from(lhs) + rhs;
    }

    /*
    template <unsigned exp2, unsigned man2,
        bool Implicit = is_implicit, estd::enable_if_t<Implicit, int> = 0>
    operator fixed_point<exp2, man2, o>() const
    {
        return fixed_point<exp2, man2, o>(*this);
    }   */
};


template <unsigned exp1, unsigned exp2, unsigned man, embr::dsp::v1::fixed_point_options o>
inline constexpr auto operator*(
    fixed_point<exp1, man, o> lhs, fixed_point<exp2, man, o> rhs) ->
#if FEATURE_EMBR_DSP_FP_PERMISSIVE_CONVERSION
    estd::enable_if_t<
        !(o & fixed_point_options::FP_IMPLICIT),
        fixed_point<estd::max(exp1, exp2), man, o>>
#else
    fixed_point<estd::max(exp1, exp2), man, o>
#endif
{
    return { estd::units::relaxed_narrow_t{}, (lhs.value() * rhs.value()) >> man };
}

#if FEATURE_EMBR_DSP_FP_PERMISSIVE_CONVERSION
// 100% experimental
template <unsigned exp1, unsigned exp2, unsigned man1, unsigned man2, fixed_point_options o>
inline constexpr auto operator*(
    fixed_point<exp1, man1, o> lhs, fixed_point<exp2, man2, o> rhs) ->
    estd::enable_if_t<
        o & fixed_point_options::FP_IMPLICIT,
        fixed_point<estd::max(exp1, exp2), man1 + man2, o>>
{
    // DEBT: Consolidate with convert_ll
    return { estd::units::relaxed_narrow_t{}, lhs.value() * rhs.value() };
}
#endif

template <unsigned exp1, unsigned exp2, unsigned man, embr::dsp::v1::fixed_point_options o>
inline constexpr fixed_point<estd::max(exp1, exp2), man, o> operator/(
    fixed_point<exp1, man, o> lhs, fixed_point<exp2, man, o> rhs)
{
    return { estd::units::relaxed_narrow_t{}, (lhs.value() << man) / rhs.value() };
}

template <unsigned exp, unsigned man, embr::dsp::v1::fixed_point_options o>
constexpr fixed_point<exp, man, o> abs(const fixed_point<exp, man, o>& v)
{
    return v.value() > 0 ? v : -v;
}

// DEBT: Add < and > and friends in the event that FP is non-FP_ENDIAN_NATIVE in which case
// the packed_word flavors of < and > won't play nice

}}}

namespace estd {

template <
    unsigned exp1, unsigned man1, embr::dsp::v1::fixed_point_options o1,
    unsigned exp2, unsigned man2, embr::dsp::v1::fixed_point_options o2>
struct common_type<
    embr::dsp::v1::fixed_point<exp1, man1, o1>,
    embr::dsp::v1::fixed_point<exp2, man2, o2>
>
{
    using options_type = embr::dsp::v1::fixed_point_options;

    // FIX: Need to static_assert endianness is the same

    static constexpr bool is_signed = o1 & options_type::FP_SIGNED | o2 & options_type::FP_SIGNED;
    static constexpr bool is_implicit = o1 & options_type::FP_IMPLICIT | o2 & options_type::FP_IMPLICIT;
    static constexpr options_type options =
        (is_signed ? options_type::FP_SIGNED : options_type::FP_UNSIGNED) |
        (is_implicit ? options_type::FP_IMPLICIT : options_type::FP_NONE);

    static constexpr unsigned exponent = estd::max(exp1, exp2);
    static constexpr unsigned mantissa = estd::max(man1, man2);

    using type = embr::dsp::v1::fixed_point<exponent, mantissa, options>;
};

}

#include <estd/internal/macro/pop.h>

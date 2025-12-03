#include <estd/algorithm.h>
#include <estd/flags.h>
#include <estd/type_traits.h>

#include <embr/fwd/type_from_bits.h>
#include <embr/internal/type_from_bits.h>

#include "fwd.h"
#include "packed.h"

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
    detail::packed_word_base<(o & FP_SIGNED) ? PACKED_WORD_SIGNED : PACKED_WORD_DEFAULT, estd::integer_sequence<unsigned, exponent_, mantissa_>>
{
    static constexpr unsigned exponent = exponent_;
    static constexpr unsigned mantissa = mantissa_;

    using base_type = detail::packed_word_base<(o & FP_SIGNED) ? PACKED_WORD_SIGNED : PACKED_WORD_DEFAULT, estd::integer_sequence<unsigned, exponent, mantissa>>;
    using base_type::channel;
    using base_type::v_;
    using typename base_type::signed_type;
    using typename base_type::unsigned_type;
    using typename base_type::value_type;
    using this_type = fixed_point;

    static constexpr bool is_signed = o & FP_SIGNED;
    static constexpr unsigned mantissa_max = 1 << mantissa;
    using promoted_type = type_from_bits_t<1 + exponent + mantissa, is_signed>;

    fixed_point() = default;
    constexpr explicit fixed_point(value_type v) :
        base_type{v}
    {}

    /*
     * Not ready yet
    template <unsigned exponent2, unsigned mantissa2>
    constexpr explicit fixed_point(const fixed_point<exponent2, mantissa2, o>& copy_from)
    {

    }   */

    ///
    /// @brief from
    /// @param v intrinsic such as a float, int, etc
    /// @return
    ///
    template <class Numeric>
    static constexpr this_type from(Numeric v)
    {
        if constexpr (estd::numeric_limits<Numeric>::is_integer)
        {
            return fixed_point(v << mantissa);
        }
        else
        {
            // -1.75
            // -1
            // -1.75 - -1 = -0.75
            // Otherwise, float
            const auto floor = static_cast<value_type>(v);
            const auto man = static_cast<value_type>(std::abs(v - floor) * mantissa_max);

            return fixed_point(floor << mantissa | man);
        }
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

    fixed_point<exponent, mantissa, o | FP_SIGNED> operator-() const
    {
        return { -exp(), man() };
    }

    constexpr bool operator==(const this_type& compare_to)
    {
        return compare_to.value() == base_type::v_;
    }

    template <unsigned exp2, fixed_point_options o2>
    constexpr bool operator==(const fixed_point<exp2, mantissa, o2>& compare_to)
    {
        return compare_to.man() == man() && compare_to.exp() == exp();
    }

    /*
    template <unsigned exp2, unsigned man2, fixed_point_options o2>
    constexpr bool operator==(const fixed_point<exp2, man2, o2>& compare_to)
    {
        return compare_to.man() == man() && compare_to.exp() == exp();
    }   */
};

// DEBT: Look into type promotion and do that here
template <unsigned num1, unsigned num2, unsigned den>
inline constexpr fixed_point<estd::max(num1, num2), den> operator*(
    fixed_point<num1, den> lhs, fixed_point<num2, den> rhs)
{
    return { (lhs.value() * rhs.value()) >> den };
}

template <unsigned exp, unsigned man, fixed_point_options o, class Integer>
inline constexpr auto operator*(fixed_point<exp, man, o> lhs, Integer rhs) ->
    estd::enable_if_t<estd::numeric_limits<Integer>::is_integer, fixed_point<exp, man>>
{
    return { lhs.value() * rhs };
}

template <unsigned exp, unsigned man, fixed_point_options o, class Integer>
inline constexpr auto operator/(fixed_point<exp, man, o> lhs, Integer rhs) ->
    estd::enable_if_t<estd::numeric_limits<Integer>::is_integer, fixed_point<exp, man>>
{
    return { lhs.value() / rhs };
}

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

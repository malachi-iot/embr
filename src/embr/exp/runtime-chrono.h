#pragma once

namespace embr { namespace experimental {

enum runtime_ratio_types
{
    runtime_ratio_both,     ///< numerator and denominator both runtime
    runtime_ratio_den,      ///< numerator compile time, denominator run time
    runtime_ratio_num       ///< numerator runtime, denominator compile time
};

template <typename Rep, Rep default_ = 1, runtime_ratio_types = runtime_ratio_both>
struct runtime_ratio;

template <typename Rep, Rep default_>
struct runtime_ratio<Rep, default_, runtime_ratio_both>
{
    const Rep num;
    const Rep den;

    ESTD_CPP_CONSTEXPR_RET runtime_ratio(Rep num, Rep den) : num(num), den(den) {}

    template <Rep val, runtime_ratio_types rrt>
    runtime_ratio(const runtime_ratio<Rep, val, rrt>& copy_from) :
        num(copy_from.num), den(copy_from.den)
    {}
};

// ratio with only a runtime denominator portion - numerator is constexpr
template <typename Rep, Rep num_ = 1>
struct runtime_divisor
{
    const Rep den;

    static CONSTEXPR Rep num = num_;
};

template <typename Rep, Rep den_ = 1>
struct runtime_multiplier
{
    const Rep num;

    ESTD_CPP_CONSTEXPR_RET runtime_multiplier(Rep num_) : num(num_) {}
    ESTD_CPP_CONSTEXPR_RET runtime_multiplier(const runtime_multiplier& copy_from) : num(copy_from.num_) {}

    // DEBT: This is cheating
    /*
    inline runtime_multiplier& operator=(const runtime_multiplier& copy_from)
    {
        return * new (this) runtime_multiplier(copy_from);
    } */

    static CONSTEXPR Rep den = den_;
};


template <typename Rep, Rep num_>
struct runtime_ratio<Rep, num_, runtime_ratio_den>
{
    const Rep den;

    static CONSTEXPR Rep num = num_;
};

template <typename Rep, Rep den_>
struct runtime_ratio<Rep, den_, runtime_ratio_num>
{

};


template <typename Rep, class TRatio = runtime_ratio<Rep> >
struct runtime_duration
{
    typedef TRatio ratio_type;
    
    ratio_type ratio_;
    Rep count_;
};

template <class Rep, Rep lhs_num, std::intmax_t rhs_num, std::intmax_t rhs_den>
inline runtime_ratio<Rep> runtime_multiply(
    const runtime_ratio<Rep, lhs_num, runtime_ratio_den>& lhs,
    estd::ratio<rhs_num, rhs_den> rhs)
{
    constexpr std::intmax_t gcd = estd::internal::gcd<lhs_num, rhs_den>::value;
    constexpr Rep reduced_lhs_num = lhs_num / gcd;
    constexpr intmax_t reduced_rhs_den = rhs_den / gcd;
    return runtime_ratio<Rep>(reduced_lhs_num * rhs.num, lhs.den * reduced_rhs_den);
    //return runtime_ratio<Rep, reduced_lhs_num * rhs.num, runtime_ratio_den>(lhs.den * reduced_rhs_den);
}

template <class Rep, Rep lhs_den, std::intmax_t rhs_num, std::intmax_t rhs_den>
inline runtime_ratio<Rep> runtime_multiply(
    const runtime_ratio<Rep, lhs_den, runtime_ratio_num>& lhs,
    estd::ratio<rhs_num, rhs_den> rhs)
{
    constexpr std::intmax_t gcd = estd::internal::gcd<lhs_den, rhs_num>::value;
    constexpr Rep reduced_lhs_den = lhs_den / gcd;
    constexpr intmax_t reduced_rhs_num = rhs_num / gcd;
    return runtime_ratio<Rep>(lhs.num * reduced_rhs_num, reduced_lhs_den * rhs.den);
    //return runtime_ratio<Rep, reduced_lhs_num * rhs.num, runtime_ratio_den>(lhs.den * reduced_rhs_den);
}



template <class Rep, Rep lhs_den, std::intmax_t rhs_num, std::intmax_t rhs_den>
ESTD_CPP_CONSTEXPR_RET runtime_ratio<Rep> runtime_divider(
    const runtime_multiplier<Rep, lhs_den>& lhs,
    estd::ratio<rhs_num, rhs_den> rhs)
{
    return runtime_ratio<Rep>(lhs.num, lhs.den);
}

// https://www.wolframalpha.com/input?i2d=true&i=Divide%5Bh%2Cd%5D%3Df*Divide%5By%2Cx%5D
// h = hz, d = divisor, x = numerator of chrono ratio, y = denomenator of chrono ratio,
// h/d = number of ticks to form one esp32 counter second
// y/x = number of ticks to form one chrono second
// f = factor to convert y/x into h/d
// f = (x/y)

// This exists so that we can convert timepoints to
// esp32 timer native 64-bit divided value.  Note that this is a kind of
// underpinning for a potentially bigger runtime_ratio / runtime_duration codebase
template <typename TInt, int numerator_ = -1, int denominator_ = 80000000>
struct DurationConverter;

template <typename TInt, int denominator_>
struct DurationConverter<TInt, -1, denominator_>
{
    typedef TInt int_type;
    typedef runtime_multiplier<uint32_t, denominator_> period_type;
    period_type ratio_{80};
    void numerator(uint32_t n)
    {
        new (&ratio_) period_type(n);
    }
    ESTD_CPP_CONSTEXPR_RET const period_type& ratio() const { return ratio_; }
    /*
    uint32_t numerator_ = 80;
    uint32_t numerator() const { return numerator_; }
    static constexpr int denominator() { return denominator_; } */
    // bits of precision to lose in order to avoid overflow/underflow
    static constexpr unsigned adjuster() { return 2;}

    template <typename Rep, typename Period>
    int_type convert(const estd::chrono::duration<Rep, Period>& convert_from) const
    {
        // NOTE: timor divisor generally represents 80MHz (APB) / divisor()
        constexpr int precision_helper = adjuster();
        constexpr int_type hz = ratio_.den << precision_helper;
        int_type den = hz / ratio_.num;    // aka timer counts per second
        int_type mul = Period::num * den / Period::den;
        return (convert_from.count() * mul) >> precision_helper;
    }

    // https://bit.ly/3TQ6AJ2
    template <typename Rep, typename Period>
    estd::chrono::duration<Rep, Period>& convert(int_type convert_from, estd::chrono::duration<Rep, Period>* convert_to) const
    {
        typedef typename estd::ratio<Period::den, denominator_ * Period::num>::type r;
        constexpr auto num = r::num;
        constexpr auto den = r::den;
        Rep raw = ratio_.num * convert_from * num / den;
        return * new (convert_to) estd::chrono::duration<Rep, Period>(raw);
    }
};

template <typename TInt, int numerator_, int denominator_>
struct DurationConverter
{
    typedef TInt int_type;
    typedef typename estd::ratio<numerator_, denominator_>::type period_type;

    static period_type ratio() { return period_type(); }

    typedef estd::chrono::duration<int_type, period_type> duration;

    template <typename Rep, typename Period>
    static constexpr int_type convert(const estd::chrono::duration<Rep, Period>& convert_from)
    {
        return duration(convert_from).count();
    }

    template <typename Rep, typename Period>
    static constexpr estd::chrono::duration<Rep, Period>& convert(int_type convert_from,
        estd::chrono::duration<Rep, Period>* convert_to)
    {
        return * new (convert_to) estd::chrono::duration<Rep, Period>(duration(convert_from));
    }
};


}}
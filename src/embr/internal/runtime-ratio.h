#pragma once

#include <estd/chrono.h>

// DEBT: Combine this with the j1939 embr units code

namespace embr { namespace internal {

enum runtime_ratio_types
{
    runtime_ratio_both,     ///< numerator and denominator both runtime
    runtime_ratio_den,      ///< numerator compile time, denominator run time
    runtime_ratio_num       ///< numerator runtime, denominator compile time
};

// precision_assist is EXPERIMENTAL and behavior varies depending on specialization
template <typename Rep, Rep default_ = 1, runtime_ratio_types = runtime_ratio_both, unsigned precision_assist = 0>
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

// EXPERIMENTAL naming
template <typename Rep, Rep den>
using runtime_numerator_ratio = runtime_ratio<Rep, den, runtime_ratio_num>;

template <typename Rep, Rep den>
using runtime_denominator_ratio = runtime_ratio<Rep, den, runtime_ratio_den>;

// Lifted from https://www.geeksforgeeks.org/euclidean-algorithms-basic-and-extended/
template <class T>
constexpr T gcd(T a, T b)
{
    return (a == 0) ? b : gcd(b % a, a);
}


// (disabled) precision assist shifts to the right:
// lhs numerator POST reduction
// rhs denominator POST reduction
template <typename Rep, Rep num_, unsigned precision_assist>
struct runtime_ratio<Rep, num_, runtime_ratio_den, precision_assist>
{
    const Rep den;

    constexpr runtime_ratio(Rep den) : den(den) {}

    static CONSTEXPR Rep num = num_;

    // reduce the left (this) numerator against the right denominator
    template <Rep rhs_den>
    using mult_reducer = typename estd::ratio<num_, rhs_den>::type;

    // reduces left numerator against right denominator, then multiplies by rhs_num
    template <Rep rhs_num, Rep rhs_den>
    static constexpr Rep mult_helper()
    {
        return mult_reducer<rhs_den>::num * rhs_num;
        //return mult_reducer<rhs_den>::num >> precision_assist * rhs_num;
    };


    template <std::intmax_t rhs_num, std::intmax_t rhs_den>
    using mult_ret_type = runtime_ratio<Rep, mult_helper<rhs_num, rhs_den>(), runtime_ratio_den>;

    template <std::intmax_t rhs_num, std::intmax_t rhs_den>
    constexpr runtime_ratio<Rep, mult_helper<rhs_num, rhs_den>(), runtime_ratio_den>
        multiply(estd::ratio<rhs_num, rhs_den> rhs) const
    {
        typedef mult_reducer<rhs_den> reducer;
        return runtime_ratio<
            Rep,
            mult_helper<rhs_num, rhs_den>(),
            runtime_ratio_den>
        {(Rep)(den * reducer::den)};
        //{(Rep)(den * reducer::den >> precision_assist)};
        //return runtime_ratio<Rep, reducer::num * rhs_num, runtime_ratio_den>{(Rep)(den * reducer::den)};
        //return runtime_ratio<Rep, reduced_lhs_num * rhs.num, runtime_ratio_den>(lhs.den * reduced_rhs_den);
    }

    template <typename Rep2, Rep2 rhs_den>
    constexpr runtime_ratio<Rep> multiply(const runtime_ratio<Rep2, rhs_den, runtime_ratio_num>& rhs) const
    {
        typedef mult_reducer<rhs_den> reducer;
        return runtime_ratio<Rep>(reducer::num * rhs.num, reducer::den * den);
    }

    constexpr runtime_ratio<Rep, num_, runtime_ratio_num> inverse() const
    {
        return runtime_ratio<Rep, num_, runtime_ratio_num>(den);
    }
};

template <typename Rep, Rep den_>
struct runtime_ratio<Rep, den_, runtime_ratio_num>
{
    const Rep num;

    static CONSTEXPR Rep den = den_;

    // reduce the left (this) denominator against the right numerator
    template <Rep rhs_num>
    using mult_reducer = typename estd::ratio<rhs_num, den_>::type;

    // reduces left denominator against right numerator,
    // then multiplies that left denominator by rhs_den
    template <Rep rhs_num, Rep rhs_den>
    static constexpr Rep mult_helper()
    {
        return mult_reducer<rhs_num>::den * rhs_den;
    };

    template <std::intmax_t rhs_num, std::intmax_t rhs_den>
    using mult_ret_type = runtime_ratio<Rep, mult_helper<rhs_num, rhs_den>(), runtime_ratio_num>;

    ESTD_CPP_CONSTEXPR_RET runtime_ratio(Rep num) : num(num) {}


    template <std::intmax_t rhs_num, std::intmax_t rhs_den>
    constexpr runtime_ratio<Rep, mult_helper<rhs_num, rhs_den>(), runtime_ratio_num>
        multiply(estd::ratio<rhs_num, rhs_den>) const
    {
        typedef mult_reducer<rhs_num> reducer;
        return runtime_ratio<
            Rep,
            mult_helper<rhs_num, rhs_den>(),
            runtime_ratio_num>
        {(Rep)(num * reducer::num)};
    }

    // UNTESTED
    template <typename Rep2, Rep2 rhs_num>
    constexpr runtime_ratio<Rep> multiply(const runtime_ratio<Rep2, rhs_num, runtime_ratio_den>& rhs) const
    {
        typedef mult_reducer<rhs_num> reducer;
        return runtime_ratio<Rep>(reducer::num * num, reducer::den * rhs.den);
    }

    constexpr runtime_ratio<Rep, den_, runtime_ratio_den> inverse() const
    {
        return runtime_ratio<Rep, den_, runtime_ratio_den>(num);
    }
};

template <typename Rep, Rep default_, runtime_ratio_types rrt>
runtime_ratio<Rep> reduce(const runtime_ratio<Rep, default_, rrt>& v)
{
    const Rep divisor = gcd(v.num, v.den);

    return runtime_ratio<Rep>(v.num / divisor, v.den / divisor);
}

// multiplies by integer (or float maybe?) and returns the same
// DEBT - naming and location not ideal
template <typename Rep, Rep default_, runtime_ratio_types rrt>
constexpr Rep integer_multiply(const runtime_ratio<Rep, default_, rrt> ratio, Rep v)
{
    return ratio.num * v / ratio.den;
}



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
    typedef typename estd::ratio<lhs_num, rhs_den>::type reducer;
    return runtime_ratio<Rep>(reducer::num * rhs.num, lhs.den * reducer::den);
    //return runtime_ratio<Rep, reduced_lhs_num * rhs.num, runtime_ratio_den>(lhs.den * reduced_rhs_den);
}

// EXPERIMENTAL
template <class TFrom, class TTo>
struct ratio_converter;

template <typename TFromRep, TFromRep num, std::intmax_t rhs_num, std::intmax_t rhs_den>
struct ratio_converter<runtime_ratio<TFromRep, num, runtime_ratio_den>, estd::ratio<rhs_num, rhs_den> >
{
    typedef runtime_ratio<TFromRep, num, runtime_ratio_den> lhs_type;
    typedef runtime_ratio<TFromRep, num, runtime_ratio_num> inverse_lhs_type;
    lhs_type lhs;
    typedef estd::ratio<rhs_num, rhs_den> rhs_type;
    //typedef typename inverse_lhs_type::template mult_ret_type<rhs_num, rhs_den> converter_type;
    typedef decltype(lhs.inverse().multiply(rhs_type{})) converter_type;
    converter_type converter;

    // gets numerator... right?
    static TFromRep init_helper(const lhs_type& lhs)
    {
        auto inverse = lhs.inverse();
        auto r = inverse.multiply(rhs_type{});

        return r.num;
    }

    ratio_converter(TFromRep lhs_num) :
        lhs(lhs_num),
        converter(init_helper(lhs))
    {}

    template <typename T>
    T lhs_to_rhs(T v)
    {
        auto v2 = integer_multiply(converter, (TFromRep)v);
        return v2;
        /*
        auto inverse = lhs.inverse();
        auto r = inverse.multiply(rhs_type{});
        return integer_multiply(r, (TFromRep)v); */
    }
};


template <typename TFromRep, TFromRep den, std::intmax_t rhs_num, std::intmax_t rhs_den>
struct ratio_converter<runtime_ratio<TFromRep, den, runtime_ratio_num>, estd::ratio<rhs_num, rhs_den> >
{
    typedef runtime_ratio<TFromRep, den, runtime_ratio_num> lhs_type;
    lhs_type lhs;
    typedef estd::ratio<rhs_num, rhs_den> rhs_type;
    //typedef typename inverse_lhs_type::template mult_ret_type<rhs_num, rhs_den> converter_type;
    typedef decltype(lhs.inverse().multiply(rhs_type{}).inverse()) converter_type;
    converter_type converter;

    static TFromRep init_helper(const lhs_type& lhs)
    {
        auto inverse = lhs.inverse();
        auto r = inverse.multiply(rhs_type{}).inverse();

        return r.num;
    }

    ratio_converter(TFromRep lhs_num) :
        lhs(lhs_num),
        converter(init_helper(lhs))
    {}

    template <typename T>
    T lhs_to_rhs(T v)
    {
        auto v2 = integer_multiply(converter, (TFromRep)v);
        return v2;
    }
};



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
    static ESTD_CPP_CONSTEXPR_RET int denominator() { return denominator_; }

    typedef TInt int_type;
    typedef runtime_numerator_ratio<int_type, denominator_> period_type;
    period_type ratio_;
    void numerator(TInt n)
    {
        new (&ratio_) period_type(n);
    }
    ESTD_CPP_CONSTEXPR_RET const period_type& ratio() const { return ratio_; }

    // DEBT: Remove default and pass in v
    DurationConverter(TInt v = 80) : ratio_(v) {}

    /*
    uint32_t numerator_ = 80;
    uint32_t numerator() const { return numerator_; }
    static constexpr int denominator() { return denominator_; } */
    // bits of precision to lose in order to avoid overflow/underflow
    static constexpr unsigned adjuster() { return 2;}

    // convert from duration to ticks
    template <typename Rep, typename Period>
    int_type convert(const estd::chrono::duration<Rep, Period>& convert_from) const
    {
        // NOTE: timor divisor generally represents 80MHz (APB) / divisor()
        constexpr int precision_helper = adjuster();
        constexpr int_type hz = denominator() << precision_helper;
        int_type den = hz / ratio_.num;    // aka timer counts per second
        int_type mul = Period::num * den / Period::den;
        return (convert_from.count() * mul) >> precision_helper;
    }

    // convert from ticks to duration
    // https://bit.ly/3TQ6AJ2
    template <typename Rep, typename Period>
    estd::chrono::duration<Rep, Period>& convert(int_type convert_from, estd::chrono::duration<Rep, Period>* convert_to) const
    {
        /* Experimenting, doesn't quite work
        auto r2 = ratio_.inverse();
        auto rr2 = r2.multiply(Period{});
        Rep _convert_to = integer_multiply(rr2, convert_from);
         */

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

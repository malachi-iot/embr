#pragma once

namespace embr { namespace experimental {

template <typename Rep>
struct runtime_ratio
{
    const Rep num_;
    const Rep den_;

    const Rep& num() const { return num_; }
    const Rep& den() const { return den_; }
};

// ratio with only a runtime denominator portion - numerator is constexpr
template <typename Rep, Rep num_ = 1>
struct runtime_divisor
{
    const Rep den_;

    static ESTD_CPP_CONSTEXPR_RET Rep num() { return num_; }
    const Rep& den() const { return den_; }
};

template <typename Rep, Rep den_ = 1>
struct runtime_multiplier
{
private:
    const Rep num_;

public:
    ESTD_CPP_CONSTEXPR_RET runtime_multiplier(Rep num_) : num_(num_) {}
    ESTD_CPP_CONSTEXPR_RET runtime_multiplier(const runtime_multiplier& copy_from) : num_(copy_from.num_) {}

    // DEBT: This is cheating
    inline runtime_multiplier& operator=(const runtime_multiplier& copy_from)
    {
        return * new (this) runtime_multiplier(copy_from);
    }

    const Rep& num() const { return num_; }
    static ESTD_CPP_CONSTEXPR_RET Rep den() { return den_; }
};


template <typename Rep, class TRatio = runtime_ratio<Rep> >
struct runtime_duration
{
    typedef TRatio ratio_type;
    
    ratio_type ratio_;
    Rep count_;
};

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
        constexpr int_type hz = ratio_.den() << precision_helper;
        int_type den = hz / ratio_.num();    // aka timer counts per second
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
        Rep raw = ratio_.num() * convert_from * num / den;
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
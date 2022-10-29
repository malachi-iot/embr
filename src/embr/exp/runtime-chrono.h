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

    Rep num() { return num; }
    const Rep& den() const { return den_; }
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
    uint32_t numerator_ = 80;
    uint32_t numerator() const { return numerator_; }
    static constexpr int denominator() { return denominator_; }
    // bits of precision to lose in order to avoid overflow/underflow
    static constexpr unsigned adjuster() { return 2;}

    template <typename Rep, typename Period>
    int_type convert(const estd::chrono::duration<Rep, Period>& convert_from) const
    {
        // NOTE: timor divisor generally represents 80MHz (APB) / divisor()
        constexpr int precision_helper = adjuster();
        constexpr int_type hz = denominator_ << precision_helper;
        int_type den = hz / numerator();    // aka timer counts per second
        int_type mul = Period::num * den / Period::den;
        return (convert_from.count() * mul) >> precision_helper;
    }

    // https://bit.ly/3TQ6AJ2
    template <typename Rep, typename Period>
    estd::chrono::duration<Rep, Period>& convert(int_type convert_from, estd::chrono::duration<Rep, Period>* convert_to) const
    {
        //typedef estd::ratio<Period::den, denominator_ * Period::num> r;       // FIX: Ours doesn't do reduction
        typedef std::ratio<Period::den, denominator_ * Period::num> r;
        constexpr auto num = r::num;
        constexpr auto den = r::den;
        Rep raw = numerator() * convert_from * num / den;
        return * new (convert_to) estd::chrono::duration<Rep, Period>(raw);
    }
};

template <typename TInt, int numerator_, int denominator_>
struct DurationConverter
{
    typedef TInt int_type;

    static constexpr int numerator() { return numerator_; }

    typedef estd::chrono::duration<int_type, estd::ratio<numerator_, denominator_> > duration;

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
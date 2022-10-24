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
struct TimerSchedulerConverter
{
    uint32_t divisor_ = 80;

    uint32_t divisor() const { return divisor_; }

    template <typename Rep, typename Period>
    uint64_t convert(const estd::chrono::duration<Rep, Period>& convert_from)
    {
        // NOTE: timor divisor generally represents 80MHz (APB) / divisor()
        constexpr int precision_helper = 2;
        constexpr uint64_t hz = 80000000 << precision_helper;
        uint64_t den = hz / divisor();    // aka timer counts per second
        uint64_t mul = Period::num * den / Period::den;
        return (convert_from.count() * mul) >> precision_helper;
    }
};


}}
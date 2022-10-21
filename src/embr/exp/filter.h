#pragma once

namespace embr { namespace experimental {

template <typename T, T max_, T min = T()>
struct fake_lpf
{
    typedef T energy_type;

private:
    energy_type energy_;

public:
    static constexpr energy_type max() { return max_; }
    const energy_type& energy() const { return energy_; }

    inline bool add(energy_type v)
    {
        //if(energy + v > max)  // Overflow risk
        if(energy_ > (max_ - v))
        {
            energy_ = max_;
            return true;
        }
        else
        {
            energy_ += v;
            return false;
        }
    }

    inline bool subtract(energy_type v)
    {
        if(v > energy_)
        {
            energy_ = min;
            return false;
        }
        else
        {
            energy_ -= v;
            return true;
        }
    }
};

template <typename T>
struct filter_wrapper;

template <typename Rep, typename Period>
struct filter_wrapper<estd::chrono::duration<Rep, Period> >
{
    typedef estd::chrono::duration<Rep, Period> energy_type;

    template <Rep max>
    using filter = fake_lpf<Rep, max>;
};


}}
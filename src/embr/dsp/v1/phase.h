#pragma once

// DEBT: Do estd version of this if we can
#include <cmath>

namespace embr { namespace dsp { inline namespace v1 {

template <class Numeric>
class phase_generator
{
    using scalar_type = Numeric;
    using this_type = phase_generator;

    // The phase increment is the phase value the phasor increases by
    // per sample.  (2xNyquist?)
    scalar_type incr_;
    scalar_type phase_;

    // DEBT: Probably better if this is scalar_type
    static constexpr double two_pi = 2 * M_PI; // 2*pi = 360˚ = one full cycle

public:
    phase_generator() = default;

    constexpr explicit phase_generator(scalar_type incr, scalar_type phase = 0) :
        incr_{incr},
        phase_{phase}
    {
    }

    constexpr static scalar_type incr_from_freq(scalar_type v, unsigned hz)
    {
        return two_pi / hz * v;
    }

    static constexpr phase_generator from_freq(scalar_type freq, unsigned hz, scalar_type phase = 0)
    {
        return phase_generator(incr_from_freq(freq, hz), phase);
    }

    void freq(scalar_type v, unsigned hz)
    {
        incr_ = incr_from_freq(v, hz);
    }

    constexpr scalar_type operator*() const { return phase_; }

    constexpr scalar_type phase() const { return phase_; }

    // DEBT: Prefer not to do this
    scalar_type& operator*() { return phase_; }

    constexpr scalar_type incr() const { return incr_; }

    this_type& operator++()
    {
        phase_ += incr_;

        // 2Pi/360˚/full circle we have to reset the phase
        if (phase_ >= static_cast<scalar_type>(two_pi))
            phase_ -= static_cast<scalar_type>(two_pi);

        return *this;
    }

    scalar_type bump()
    {
        const scalar_type phase = phase_;

        operator++();

        return phase;
    }

    this_type operator++(int)
    {
        return phase_generator{ incr_, bump() };
    }
};

}}}

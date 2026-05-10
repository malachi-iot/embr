#pragma once

#include <estd/cstdlib.h>

namespace embr { namespace dsp { inline namespace v1 {

template <class Scalar>
class drc
{
    Scalar envelope_{};

public:
    struct params
    {
        Scalar threshold;

        /// Higher number = FASTER
        /// Lower number = SLOWER
        Scalar attack;
        Scalar release;
    };

    template <class Scalar2 = Scalar>
    Scalar2 process(Scalar2 in, const params&);

    void reset() { envelope_ = {}; }
};


// Good reads for a compressor proper:
// https://github.com/tierneytim/btAudio
// https://esp32.com/viewtopic.php?t=18657
// https://github.com/velipso/sndfilter?tab=readme-ov-file

// Adapted directly from
// https://github.com/espressif/esp-dsp/blob/v1.5.2/applications/lyrat_board_app/main/audio_amp_main.c
// Sure looks like *they* got it from
// https://christianfloisand.wordpress.com/2014/06/09/dynamics-processing-compressorlimiter-part-1/
template <class Scalar>
template <class Scalar2>
inline Scalar2 drc<Scalar>::process(Scalar2 in, const params& p)
{
    using std::abs;

    Scalar& env = envelope_;

    // Calculate envelope
    // DEBT: https://github.com/malachi-iot/estdlib/issues/199
    const Scalar abs_input = abs(in);
    if (abs_input > env) {
        env = env * (1 - p.attack) + p.attack * abs_input;
    } else {
        env = env * (1 - p.release) + p.release * abs_input;
    }

    // Apply compression
    return env > p.threshold ? in * (p.threshold / env) : in;
}

template <class Scalar, class It, class ItOut>
ItOut process(drc<Scalar>& d, const typename drc<Scalar>::params& p, It begin, It end, ItOut out)
{
    for(; begin != end; ++begin, ++out)
        *out = d.process(*begin, p);

    return out;
}


}}}

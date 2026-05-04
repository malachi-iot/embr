#pragma once

#include <estd/cstdlib.h>

namespace embr { namespace dsp { inline namespace v1 {

template <class Scalar>
class drc
{
    Scalar envelope_{};

public:
    Scalar process(Scalar in, Scalar threshold, Scalar attack, Scalar release);

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
inline Scalar drc<Scalar>::process(Scalar in, Scalar threshold, Scalar attack, Scalar release)
{
    Scalar& env = envelope_;

    // Calculate envelope
    // DEBT: https://github.com/malachi-iot/estdlib/issues/199
    const Scalar abs_input = std::abs(in);
    if (abs_input > env) {
        env = env * (1 - attack) + attack * abs_input;
    } else {
        env = env * (1 - release) + release * abs_input;
    }

    // Apply compression
    return env > threshold ? in * (threshold / env) : in;
}


}}}

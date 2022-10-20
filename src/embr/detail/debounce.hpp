#pragma once

#include "debounce.h"

namespace embr { namespace detail {

bool Debouncer::encountered(duration delta, States switch_to)
{
    // reaching here means we encountered a particular state A, so record
    // amount of time spent in that A state
    noise_or_signal += delta;

    if(noise_or_signal > signal_threshold())
    {
        // we have enough on energy to indicate a real signal
        // 'switch_to' is expected to be B state
        state(switch_to);
        state(Idle);
        return true;
    }

    return false;
}

inline bool Debouncer::time_passed(duration delta, bool on)
{
    // retain current state so that we can act on it in the switch statement
    Substates ss = substate();
    // retain incoming state.  NOTE 'encountered' may supersede this
    state(on ? EvalOn : EvalOff);

    if(state() == Unstarted)
    {
        state(on ? On : Off);
        noise_or_signal = duration::zero();
        return true;
    }

    switch(ss)
    {
        case Idle:
            noise_or_signal = delta;
            break;

        case EvalOn:
            if(state() == Off)
            {
                // reaching here means we went from off to on back to off, OR
                // reaching here means we went from off to on and stayed on,
                // so record amount of time spent in that on state
                return encountered(delta, On);
            }

            break;

        case EvalOff:
            if(state() == On)
            {
                // reaching here means we went from on to off back to on, OR
                // reaching here means we went from on to off and stayed off,
                // so record amount of time spent in that off state
                return encountered(delta, Off);
            }
            break;
    }

    return false;
}

}}
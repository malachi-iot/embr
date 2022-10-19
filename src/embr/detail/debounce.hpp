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
    if(state() == Unstarted)
    {
        state(on ? On : Off);
        state(on ? EvalOn : EvalOff);
        return true;
    }

    switch(substate())
    {
        case Idle:
            state(on ? EvalOn : EvalOff);
            noise_or_signal = delta;
            break;

        case EvalOn:
        {
            if(on)
            {
                if(state() == Off)
                {
                    // reaching here means we went from off to on and stayed on, so
                    // record amount of additional time spent in that on state
                    return encountered(delta, On);
                }

                return false;      // noop if same state that we previously observed
            }

            // incoming state is off
            state(EvalOff);

            // if main state is off...
            if(state() == Off)
            {
                // reaching here means we went from off to on back to off, so record
                // amount of time spent in that on state
                return encountered(delta, On);
            }

            break;
        }

        case EvalOff:
            if(!on) return false;      // noop if same state that we previously observed

            // incoming state is on
            state(EvalOn);

            // if main state is on...
            if(state() == On)
            {
                // reaching here means we went from on to off back to on, so record
                // amount of time spent in that off state
                return encountered(delta, Off);
            }
            break;
    }

    return false;
}

}}
#pragma once

#include "debounce.h"

namespace embr { namespace detail {

namespace internal {

template <class TImpl>
bool Debouncer<TImpl>::encountered(duration delta, States switch_to)
{
    // reaching here means we encountered a particular state A, so record
    // amount of time spent in that A state
    noise_or_signal += delta;

    if(noise_or_signal > impl_type::signal_threshold())
    {
        // we have enough on energy to indicate a real signal
        // 'switch_to' is expected to be B state
        state(switch_to);
        state(Idle);
        return true;
    }

    return false;
}

template <class TImpl>
bool Debouncer<TImpl>::remove_energy(duration delta)
{
    // If energy to remove exceeds energy available
    if(delta >= noise_or_signal)
    {
        // reset energy accumulator
        state(Idle);
        return false;
    }

    noise_or_signal -= delta;
    return true;
}

template <class TImpl>
bool Debouncer<TImpl>::time_passed(duration delta, bool on)
{
    // We only evaluate within a certain sliding time window.  When that
    // window elapses, reset debounce state
    if(delta > impl_type::max())
    {
        state(Idle);
        return false;
    }

    // Optimization of sorts.  If we are in idle state and incoming signal matches our
    // current signal state, don't process any further
    if(substate() == Idle)
    {
        // NOTE: substate() MAY be invalid here during 'Unstarted' condition.  That is OK
        // because we implicitly guard against that in state() == On/Off

        if (state() == On && on)
            return false;
        else if (state() == Off && !on)
            return false;
    }

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
            else
                remove_energy(delta);

            break;

        case EvalOff:
            if(state() == On)
            {
                // reaching here means we went from on to off back to on, OR
                // reaching here means we went from on to off and stayed off,
                // so record amount of time spent in that off state
                return encountered(delta, Off);
            }
            else
                remove_energy(delta);

            break;
    }

    return false;
}

}

}}
#pragma once

#include "debounce.h"

namespace embr { namespace detail {

namespace internal {

template <class TImpl>
bool Debouncer<TImpl>::encountered(duration delta, States switch_to)
{
    // DEBT: Workaround for overflow condition
    if(delta == duration::max())
    {
        state(switch_to);
        state(Idle);
        return true;
    }

    // reaching here means we encountered a particular state A, so record
    // amount of time spent in that A state
    noise_or_signal() += delta;

    if(noise_or_signal() > impl_type::signal_threshold())
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
inline bool Debouncer<TImpl>::remove_energy(const duration& delta)
{
    // If energy to remove exceeds energy available
    if(delta >= noise_or_signal())
    {
        // reset energy accumulator
        state(Idle);
        return false;
    }

    noise_or_signal() -= delta;
    return true;
}


template <class TImpl>
template <typename TRep, typename TPeriod>
inline bool Debouncer<TImpl>::time_passed(const estd::chrono::duration<TRep, TPeriod>& delta, bool on)
{
    // We only evaluate within a certain sliding time window.  When that
    // window elapses, reset debounce state
    // This also filters when 'delta' has a larger value than our duration
    // could handle in the fist place
    // DEBT: When unstarted, 'Idle' could be an invalid substate
    if(substate() != Idle && delta > impl_type::max())
    {
        state(Idle);
        return false;
    }
    else if(substate() != Idle && (delta + noise_or_signal()) > duration::max())
    {
        // DEBT: Workaround for overflow condition
        return time_passed_internal(duration::max(), on);
    }

    return time_passed_internal(delta, on);
}

template <class TImpl>
bool Debouncer<TImpl>::time_passed_internal(duration delta, bool on)
{
    // Optimization of sorts.  If we are in idle state and incoming signal matches our
    // current signal state, don't process any further
    if(substate() == Idle)
    {
        if (state() == On && on)
            return false;
        else if (state() == Off && !on)
            return false;
    }

    // retain current state so that we can act on it in the switch statement
    Substates ss = substate();
    // retain incoming state.  NOTE 'encountered' may supersede this
    state(on ? EvalOn : EvalOff);

    // evaluate current state - i.e. the one we just had before we switched to EvalOn/EvalOff
    switch(ss)
    {
        case Idle:
            // We were idling, and now we are about to start tracking an eval state
            // so zero out our measurer
            noise_or_signal() = duration::zero();
            break;

        case EvalOn:
            if(state() == Off || state() == Unstarted)
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
            if(state() == On || state() == Unstarted)
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

inline const char* to_string(DebouncerBase::States s)
{
    switch(s)
    {
        case DebouncerBase::On:         return "On";
        case DebouncerBase::Off:        return "Off";
        case DebouncerBase::Unstarted:  return "Unstarted";
        default:                        return "N/A";
    }
}


inline const char* to_string(DebouncerBase::Substates s)
{
    switch(s)
    {
        case DebouncerBase::Idle:       return "Idle";
        case DebouncerBase::EvalOn:     return "EvalOn";
        case DebouncerBase::EvalOff:    return "EvalOff";
        default:                        return "N/A";
    }
}

}


}}
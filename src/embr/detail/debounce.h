#pragma once

#include <estd/chrono.h>

namespace embr { namespace detail {

// DEBT: Consider bitmask approach rather than tracking time directly.  Pretty similar either way
class Debouncer
{
public:
    enum States
    {
        Unstarted = -1,
        Off,
        On
    };

    enum Substates
    {
        Idle,       ///< no on/off state evaluation happening
        EvalOn,     ///< last state evaluated was on
        EvalOff,    ///< last state evaluated was off
    };

    typedef estd::chrono::milliseconds duration;

private:
    struct
    {
        States state_ : 3;
        Substates substate_: 3;
    };

    // Amount of time observing blips into opposite state.
    // May or may not be noise.
    // If it passes signal_threshold, then we consider it
    // an actual indicator of signal.
    duration noise_or_signal;

    /// amount of "time energy" accumulated necessary to indicate a signal
    inline static duration signal_threshold()
    {
        return estd::chrono::milliseconds(40);
    }

    /// Delta window in which debounce operates before auto resetting
    inline static duration max()
    {
        return estd::chrono::milliseconds(150);
    }

    /// record amount of time spent in current eval state.  If time exceeds
    /// threshold, change to 'switch_to'
    /// @param delta
    /// @param switch_to
    /// @return
    bool encountered(duration delta, States switch_to);

    bool remove_energy(duration delta);

protected:
    void state(States v) { state_ = v; }
    void state(Substates v) { substate_ = v; }

public:
    Debouncer() : state_(States::Unstarted) {}
    Debouncer(bool on) : state_(on ? On : Off), substate_(Idle) {}

    //void reset(States s = States::Unstarted)
    void reset()
    {
        //state(s);
        state(Idle);
    }

    ///
    /// @param delta
    /// @param on
    /// @return true = main state changed
    bool time_passed(duration delta, bool on);

    States state() const { return state_; }
    Substates substate() const { return substate_; }
};

}}
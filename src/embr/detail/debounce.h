#pragma once

#include <estd/chrono.h>

namespace embr { namespace detail {

class Debouncer
{
public:
    enum States
    {
        Unstarted,
        On,
        Off
    };

    enum Substates
    {
        Idle,
        EvalOn,
        EvalOff,
    };

    typedef estd::chrono::milliseconds duration;

private:
    struct
    {
        States state_ : 3;
        Substates substate_: 3;
    };

    duration threshold;

    inline static duration max()
    {
        return estd::chrono::milliseconds(150);
    }

protected:
    void state(States v) { state_ = v; }
    void state(Substates v) { substate_ = v; }

public:
    Debouncer() : state_(States::Unstarted) {}
    Debouncer(bool on) : state_(on ? On : Off) {}

    void time_passed(duration delta, bool on);

    States state() const { return state_; }
    Substates substate() const { return substate_; }
};

}}
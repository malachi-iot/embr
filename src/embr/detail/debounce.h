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

private:
    struct
    {
        States state_ : 3;
        Substates substate_: 3;
    };

    estd::chrono::milliseconds threshold;

protected:
    void state(States v) { state_ = v; }
    void state(Substates v) { substate_ = v; }

public:
    Debouncer() : state_(States::Unstarted) {}

    void time_passed(estd::chrono::milliseconds delta);

    States state() const { return state_; }
    Substates substate() const { return substate_; }
};

}}
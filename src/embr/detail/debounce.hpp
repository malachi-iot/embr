#pragma once

#include "debounce.h"

namespace embr { namespace detail {

inline void Debouncer::time_passed(duration delta, bool on)
{
    if(state() == Unstarted)
    {
        state(on ? On : Off);
        state(on ? EvalOn : EvalOff);
        return;
    }

    switch(substate())
    {
        case Idle:
            state(on ? EvalOn : EvalOff);
            threshold = duration::min();
            break;

        case EvalOn:
            break;

        case EvalOff:
            break;
    }
}

}}
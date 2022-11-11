#pragma once

#include "button.h"

namespace embr { namespace detail {

// DEBT: Make this either a template or put it in a CPP
inline bool Button::evaluate(duration delta, bool down)
{
    const bool up = !down;

    switch(state())
    {
        case Idle:
            // up state gets tossed, since Idle = Up
            if(down)
            {
                click_count_ = 0;
                state(Evaluating);
                state(EvalDown);
            }
            break;

        case Evaluating:
            if(substate() == EvalDown)
            {
                if(down)
                {
                    // TODO: Look for detecting a long press here - this is the only
                    // scenario where we want to be called again with a duplicate down state
                    deciseconds current = long_press();
                    estd::chrono::seconds threshold(2);

                    current += delta;

                    long_press_ = current.count();

                    if(current >= threshold)
                    {
                        state(LongPressed);
                        return true;
                    }
                }
                else
                {   // up state
                    state(EvalUp);
                    // Look for click here, if the up action happened rather quickly (in human speed)
                    if(delta <= click_duration())
                    {
                        state(Clicked);
                        return true;
                    }
                }
            }
            else if(substate() == EvalUp)
            {
                if(down)
                {
                    state(EvalDown);
                    // TODO: Look for multi-click here
                }
                else
                {
                    // We don't expect to receive multiple up states.  But if we do,
                    // we can take the opportunity to see if it's time to go idle
                }
            }
            break;

        case Clicked:
            if(down)
            {
                state(Evaluating);
                state(EvalDown);
                // we are already in a click state, so now we're seeing if a new down
                // press happened fast enough to create a followup (double, triple, etc)
                // click state
                if(delta <= click_duration())
                {
                    // NOTE: We increase click count even if subsequent state ends up being
                    // LongPressed rather than click, since it's easy to do and useful to know
                    // if the long press is itself acting as a "final click"
                    ++click_count_;
                }
                else
                {
                    // too long a time passed, so this is not a multiclick but the start of a new
                    // click/longpress
                    click_count_ = 0;
                    long_press_ = deciseconds(delta).count();
                }
            }
            else
            {
                // up
                // after a click event we already are up, so this is a redundant event
                state(Idle);
            }
            break;

        case LongPressed:
            if(up)
            {
                state(Idle);
                return true;
            }
            else
            {   // down state
                // we're already long pressed down state - we do expect to see this too, but no
                // action is required
            }
            break;
    }
    return false;
}

}}

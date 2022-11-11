#pragma once

#include <estd/chrono.h>

namespace embr { namespace detail {

namespace internal {

// click means a press and release happened in under 500ms per cycle
// up/down state is handled elsewhere, likely by Debouncer
struct ButtonBase
{
    enum States
    {
        Idle,           ///< Unstarted or Button is not pressed (up)
        Clicked,        ///< Click was just detected
        LongPressed,    ///< Long press was just detected

        Evaluating,     ///< Evaluating what kind of up or down press we've got
    };

    enum Substates
    {
        Unstarted,  ///< just started or last eval states timed out
        EvalDown,   ///< last state evaluated was a down press
        EvalUp      ///< last state evaluated was a release
    };
};

}

namespace impl {

struct Button
{
    typedef estd::chrono::duration<uint16_t, estd::milli> duration;

    static constexpr duration click_duration()
    {
        return estd::chrono::milliseconds(500);
    }
};

}

struct Button : impl::Button,
    internal::ButtonBase
{
private:
    struct
    {
        States state_ : 3;
        Substates substate_ : 3;
        int click_count_ : 3;

        // x100ms for total of 12800ms
        unsigned long_press_ : 7;
    };

    void state(States v) { state_ = v; }
    void state(Substates v) { substate_ = v; }

    typedef estd::chrono::duration<uint8_t, estd::deci> deciseconds;

    ESTD_CPP_CONSTEXPR_RET deciseconds long_press() const
    { return deciseconds(long_press_); }

public:
    States state() const { return state_; }
    Substates substate() const { return substate_; }

    ESTD_CPP_CONSTEXPR_RET Button() :
        state_(Idle), substate_(Unstarted), click_count_(0), long_press_(0)
    {

    }

    /// @param delta amount of time passed since last call to evaluate
    /// @param down whether button is up or down
    /// @return true when 'state' changes except for 'Evaluating'
    bool evaluate(duration delta, bool down);
};

inline bool up(Button& b, Button::duration delta)
{
    return b.evaluate(delta, false);
}

inline bool down(Button& b, Button::duration delta)
{
    return b.evaluate(delta, true);
}

}}
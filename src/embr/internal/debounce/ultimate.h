#pragma once

#include <estd/limits.h>
#include <estd/type_traits.h>

#include "reference.h"

#include <estd/internal/macro/push.h>

/**
 * Implementation of https://hackaday.com/2015/12/10/embed-with-elliot-debounce-your-noisy-buttons-part-ii/
 * Adaptation of https://github.com/craftmetrics/esp32-button
 *
 * General idea is this is a tri-state "pattern match" debouncer:
 * eval on: just noticed an ON condition
 * eval off: just noticed an OFF condition
 * undecided: mid-evaluation, neither condition noticed
 *
 * Specifics
 * eval on: repeats frequently as long as on condition is noticed
 * eval off: present only if preceded by state on
 *
 */


namespace embr { namespace internal {

// DEBT: Since this is a low level creature, add a substate T too
template <class T, unsigned state_bits, unsigned user_bits = 0, class User = T>
struct StateHelper
{
    typedef estd::numeric_limits<T> limits;

    // It is expected state_bits fit in one byte, but not so with
    // user bits, so we arrange user bits first so that we don't
    // hop a byte when allocating state_
    User user_ : user_bits;
    T state_ : state_bits;

    ESTD_CPP_CONSTEXPR_RET EXPLICIT
    StateHelper(T state = T()) : state_(state) {}
};


template <class T, unsigned state_bits>
struct StateHelper<T, state_bits, 0, T>
{
    T state_;

    ESTD_CPP_CONSTEXPR_RET EXPLICIT
    StateHelper(T state = T()) : state_(state) {}
};

template <class T, unsigned state_bits, unsigned user_bits, class T2>
inline bool state(StateHelper<T, state_bits, user_bits>& sh, T2 v)
{
    if(sh.state_ == v) return false;

    sh.state_ = v;
    return true;
}


}}

namespace embr { namespace debounce {

inline namespace v1 { inline namespace ultimate {

template <class>
struct HistoryTraits;


template <>
struct HistoryTraits<uint8_t>
{
    static CONSTEXPR uint8_t mask_on = 0x07;        // 0b00000111
    static CONSTEXPR uint8_t mask_off = 0xC0;       // 0b11000000
    static CONSTEXPR uint8_t mask = mask_on | mask_off;
};

template <>
struct HistoryTraits<uint16_t>
{
    static CONSTEXPR uint16_t mask_on = 0x003F;     // 0b0000000000111111;
    static CONSTEXPR uint16_t mask_off = 0xF000;    // 0b1111000000000000;
    static CONSTEXPR uint16_t mask = mask_on | mask_off;
};


template <class Unsigned, class Traits = HistoryTraits<Unsigned> >
class History
{
    typedef estd::numeric_limits<Unsigned> limits;
    typedef Traits traits_type;

protected:
    Unsigned history;

    Unsigned masked() const
    {
        return history & traits_type::mask;
    }

public:
    ESTD_CPP_CONSTEXPR_RET EXPLICIT
    History() : history(0) {}

    void update(unsigned level)
    {
        history <<= 1;
        history |= level;
    }

    bool on() const
    {
        return history == limits::max();
    }

    bool off() const
    {
        return history == 0;
    }

    /// @brief Update state - did we notice an ON condition?
    /// @return
    bool eval_on()
    {
        if(masked() == traits_type::mask_on)
        {
            history = limits::max();
            return true;
        }
        return false;
    }

    /// @brief Update state - did we notice an OFF condition?
    /// @return
    bool eval_off()
    {
        if(masked() == traits_type::mask_off)
        {
            history = 0;
            return true;
        }
        return false;
    }
};



template <class Unsigned, class Traits>
States eval(History<Unsigned, Traits>& dbh)
{
    if(dbh.eval_on()) return States::Pressed;
    if(dbh.eval_off()) return States::Released;
    return States::Undefined;
}



template <class Unsigned, bool inverted = false, unsigned user_storage = 0>
class DebouncerTracker
{
    History<Unsigned> history;

    internal::StateHelper<uint8_t, 2, user_storage> storage;

public:
    ESTD_CPP_CONSTEXPR_RET DebouncerTracker() : storage(0)
    {}

    States state() const
    {
        return (States)storage.state_;
    }

    /// @brief Evaluate periodic incoming level and indicate whether
    /// an on/off state change occurred
    /// @return did we change from ON to OFF or vice versa
    bool eval(unsigned level)
    {
        if(inverted)   level = !level;

        history.update(level);

        if(history.eval_on())
        {
            // on state noticed, if we noticed that before, indicate
            // no change
            return internal::state(storage, 1);
        }
        else if(history.eval_off())
        {
            // off state noticed, if we noticed that before, indicate
            // no change
            return internal::state(storage, 2);
        }
        else
            // eval_xx is in an intermediate state = no noticed change
            return false;
    }
};

}}

}}


#include <estd/internal/macro/pop.h>
